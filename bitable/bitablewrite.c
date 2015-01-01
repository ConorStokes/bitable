#include "bitablewrite.h"
#include "bitableshared.h"
#include "writablefile.h"
#include <memory.h>
#include <assert.h>

typedef struct BufferedFile
{

    BitableWritableFile* file;
    uint8_t* buffer;

} BufferedFile;

typedef struct LeafLevel
{

    uint64_t leafPageCount;
    BufferedFile bufferedFile;
    uint64_t* initialIndice;
    int32_t* itemCount; // the number of items in the current node.
    BitableLeafIndice* itemIndices;
    uint16_t leftSize; // amount that has been allocated on the left of the node in memory (header and index into node key/data table)
    uint16_t rightSize; // amount that has been allocated on the right of the node in memory (node key/data table)

} LeafLevel;

typedef struct BranchLevel
{

    uint64_t childPageCount;
    BufferedFile bufferedFile;
    uint64_t* initialChildPage;
    uint16_t* itemCount; // the number of items in the current node.
    BitableBranchIndice* childIndices;
    uint16_t leftSize; // amount that has been allocated on the left of the node in memory (header and index into node key/data table)
    uint16_t rightSize; // amount that has been allocated on the right of the node in memory (node key/data table)

} BranchLevel;

typedef struct BitableWritable
{

    uint64_t itemCount;
    uint64_t largeValueStoreSize;

    LeafLevel leafLevel;
    BranchLevel branchLevels[ BITABLE_MAX_BRANCH_LEVELS ];
    BufferedFile largeValueFile;
    BitablePaths paths;

    uint32_t depth;
    uint16_t pageSize;
    uint16_t keyAlignment;
    uint16_t valueAlignment;

} BitableWritable;


static BitableResult create_buffered_file( BufferedFile* bufferedFile, const char* path, uint16_t pageSize )
{
    BitableResult result = bitable_wf_create( &bufferedFile->file, path );

    if ( result != BR_SUCCESS )
    {
        bufferedFile->file = NULL;
        return result;
    }

    bufferedFile->buffer = calloc( pageSize, sizeof( uint8_t ) );

    return BR_SUCCESS;
}

static void cleanup_buffered( BufferedFile* bufferedFile )
{
    if ( bufferedFile->buffer != NULL )
    {
        free( bufferedFile->buffer );
        bufferedFile->buffer = NULL;
    }

    if ( bufferedFile->file != NULL )
    {
        bitable_wf_close( bufferedFile->file );
        bufferedFile->file = NULL;
    }
}

static void cleanup_writable( BitableWritable* table )
{
    int where;

    bitable_free_paths( &table->paths );
    cleanup_buffered( &table->largeValueFile );
    cleanup_buffered( &table->leafLevel.bufferedFile );

    for ( where = 0; where < BITABLE_MAX_BRANCH_LEVELS; ++where )
    {
        cleanup_buffered( &table->branchLevels[ where ].bufferedFile );
    }

    memset( table, 0, sizeof( BitableWritable ) );
}

static BitableResult add_page_to_branch( BitableWritable* table, const BitableValue* key, uint32_t depth )
{
    BitableResult result;

    if ( table->depth > BITABLE_MAX_BRANCH_LEVELS )
    {
        return BR_MAXIMUM_TABLE_TREE_DEPTH;
    }

    {
        BranchLevel*  branchLevel = table->branchLevels + depth;
        BufferedFile* branchFile  = &branchLevel->bufferedFile;

        // note, we should only ever increment depth by 1, so we should always be adding the current
        if ( branchFile->file == NULL )
        {
            assert( depth == table->depth );

            result =
                create_buffered_file( branchFile,
                                    table->paths.branchPaths[ depth ],
                                    table->pageSize );

            if ( result != BR_SUCCESS )
            {
                return result;
            }

            table->depth = depth + 1;

            branchLevel->initialChildPage  = (uint64_t*)branchFile->buffer;
            branchLevel->itemCount         = (uint16_t*)( branchLevel->initialChildPage + 1 );
            branchLevel->childIndices      = (BitableBranchIndice*)( (uint16_t*)branchLevel->itemCount + 1 );

            // when we start a new level we add 2 items - the first node of the previous level (which doesn't need it's key stored)
            // and the second node of the previous level (just added) that does.
            *branchLevel->initialChildPage = 0;
            *branchLevel->itemCount        = 2;
            branchLevel->childPageCount    = 2;
            branchLevel->leftSize          = sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( BitableBranchIndice );
            branchLevel->rightSize         = ( key->size + ( table->keyAlignment - 1 ) ) & ~( table->keyAlignment - 1 );

            {
                uint16_t             keyOffset      = table->pageSize - branchLevel->rightSize;
                void*                keyDestination = (uint8_t*)branchFile->buffer + keyOffset;
                BitableBranchIndice* keyIndice      = branchLevel->childIndices;

                keyIndice->itemOffset = keyOffset;
                keyIndice->keySize    = (uint16_t)key->size;

                memcpy( keyDestination, key->data, key->size );
            }
        }
        else
        {
            uint16_t newLeftSize  = branchLevel->leftSize + sizeof( BitableBranchIndice );
            uint16_t newRightSize = ( branchLevel->rightSize + key->size + ( table->keyAlignment - 1 ) ) & ~( table->keyAlignment - 1 );

            if ( newLeftSize + newRightSize > table->pageSize )
            {
                result = bitable_wf_write( branchFile->file, branchFile->buffer, table->pageSize );

                if ( result != BR_SUCCESS )
                {
                    return result;
                }

                result = add_page_to_branch( table, key, depth + 1 );

                if ( result != BR_SUCCESS )
                {
                    return result;
                }

                *branchLevel->initialChildPage += branchLevel->childPageCount;
                *branchLevel->itemCount         = 1;
                branchLevel->childPageCount     = 1;
                branchLevel->leftSize           = sizeof( uint64_t ) + sizeof( uint16_t );
                branchLevel->rightSize          = 0;
            }
            else
            {
                uint16_t             keyOffset      = table->pageSize - newRightSize;
                void*                keyDestination = (uint8_t*)branchFile->buffer + keyOffset;
                BitableBranchIndice* keyIndice      = branchLevel->childIndices + ( *branchLevel->itemCount - 1 );

                keyIndice->itemOffset = keyOffset;
                keyIndice->keySize    = (uint16_t)key->size;

                memcpy( keyDestination, key->data, key->size );

                branchLevel->childPageCount += 1;
                *branchLevel->itemCount     += 1;
                branchLevel->leftSize        = newLeftSize;
                branchLevel->rightSize       = newRightSize;
            }
        }
    }

    return BR_SUCCESS;
}

BitableWritable* bitable_write_allocate()
{
    return calloc( 1, sizeof( BitableWritable ) );
}

BitableResult bitable_write_create( BitableWritable* table, const char* path, uint16_t pageSize, uint16_t keyAlignment, uint16_t dataAlignment )
{
    BitableResult result;

    if ( table->leafLevel.bufferedFile.buffer != NULL )
    {
        return BR_ALREADY_OPEN;
    }

    // check page size is in the correct range and a power of 2
    if ( pageSize < BITABLE_MIN_PAGE_SIZE || pageSize > BITABLE_MAX_PAGE_SIZE || ( pageSize & ( pageSize - 1 ) ) > 0 )
    {
        return BR_PAGESIZE_INVALID;
    }

    // check page size is in the correct range and a power of 2
    if ( keyAlignment < 1 ||
         dataAlignment < 1 ||
         keyAlignment > BITABLE_MAX_ALIGNMENT ||
         keyAlignment > BITABLE_MAX_ALIGNMENT ||
         ( keyAlignment & ( keyAlignment - 1 ) ) > 0 ||
         ( dataAlignment & ( dataAlignment - 1 ) ) > 0 )
    {
        return BR_ALIGNMENT_INVALID;
    }

    table->pageSize       = pageSize;
    table->keyAlignment   = keyAlignment;
    table->valueAlignment = dataAlignment;
    table->itemCount      = 0;
    table->depth          = 0; // this will be incremented when the first branch level is added.

    bitable_build_paths( &table->paths, path );

    {
        LeafLevel* leafLevel = &table->leafLevel;

        result = create_buffered_file( &leafLevel->bufferedFile, table->paths.leafPath, pageSize );

        // write out space for the header.
        bitable_wf_write( leafLevel->bufferedFile.file, leafLevel->bufferedFile.buffer, pageSize );

        leafLevel->leafPageCount   = 1;

        leafLevel->initialIndice   = (uint64_t*)leafLevel->bufferedFile.buffer;
        leafLevel->itemCount       = (int32_t*)( leafLevel->initialIndice + 1 );
        leafLevel->itemIndices     = (BitableLeafIndice*)( leafLevel->itemCount + 1 );

        // when we start a new level we add 2 items - the first node of the previous level (which doesn't need it's key stored)
        // and the second node of the previous level (just added) that does.
        *leafLevel->itemCount  = 0;
        leafLevel->leftSize = sizeof( uint64_t ) + sizeof( int32_t );
        leafLevel->rightSize = 0;
    }

    if ( result != BR_SUCCESS )
    {
        cleanup_writable( table );
        return result;
    }

    return BR_SUCCESS;
}

BitableResult bitable_append( BitableWritable* table, const BitableValue* key, const BitableValue* data )
{
    LeafLevel*        leafLevel         = &table->leafLevel;
    BufferedFile*     leafFile          = &leafLevel->bufferedFile;
    uint16_t          newLeftSize       = leafLevel->leftSize + ( sizeof( BitableLeafIndice ) );
    uint16_t          newKeyAllocation;
    uint16_t          newRightSize;
    BitableResult result;
    
    if ( key->size < 0 || key->size > BITABLE_MAX_KEY_SIZE )
    {
        return BR_KEY_INVALID;
    }

    newKeyAllocation = ( leafLevel->rightSize + key->size + ( table->keyAlignment - 1 ) ) & ~( table->keyAlignment - 1 );

    if ( data->size <= BITABLE_MAX_KEY_SIZE )
    {
        newRightSize = ( newKeyAllocation + data->size + ( table->valueAlignment - 1 ) ) & ~( table->valueAlignment - 1 );
    }
    else
    {
        newRightSize = ( newKeyAllocation + sizeof( uint64_t ) + ( sizeof( uint64_t ) - 1 ) ) & ~( sizeof( uint64_t ) - 1 );
    }

    // leaf page would overflow putting in this data, write the page and start a new one.
    if ( newLeftSize + newRightSize > table->pageSize )
    {
        BufferedFile* leafFile = &leafLevel->bufferedFile;

        result = bitable_wf_write( leafFile->file, leafFile->buffer, table->pageSize );

        if ( result != BR_SUCCESS )
        {
            return result;
        }

        newLeftSize =
            sizeof( uint64_t ) +
            sizeof( int32_t ) +
            sizeof( BitableLeafIndice ); // allocate at least the header and one indice

        newKeyAllocation = ( key->size + ( table->keyAlignment - 1 ) ) & ~( table->keyAlignment - 1 );

        if ( data->size <= BITABLE_MAX_KEY_SIZE )
        {
            newRightSize = ( newKeyAllocation + data->size + ( table->valueAlignment - 1 ) ) & ~( table->valueAlignment - 1 );
        }
        else
        {
            newRightSize = ( newKeyAllocation + sizeof( uint64_t ) + ( sizeof( uint64_t ) - 1 ) ) & ~( sizeof( uint64_t ) - 1 );
        }

        result = add_page_to_branch( table, key, 0 );

        if ( result != BR_SUCCESS )
        {
            return result;
        }

        *leafLevel->itemCount = 0;

        ++leafLevel->leafPageCount;
    }

    if ( data->size <= BITABLE_MAX_KEY_SIZE )
    {
        if ( data->size > 0 )
        {
            void* dataDestination = leafFile->buffer + ( table->pageSize - newRightSize );

            memcpy( dataDestination, data->data, data->size );
        }
    }
    else
    {
        uint64_t paddedStoreOffset = ( table->largeValueStoreSize + ( table->valueAlignment - 1 ) ) & ~( table->valueAlignment - 1 );

        if ( table->largeValueFile.file == NULL )
        {
            result = create_buffered_file( &table->largeValueFile, table->paths.largeValuePath, table->pageSize );

            if ( result != BR_SUCCESS )
            {
                return result;
            }
        }
        
        // If the new data won't fit in the current page in the large value store, pad out to pagesize alignment
        if ( ( ( paddedStoreOffset & ( table->pageSize - 1 ) ) + data->size ) > table->pageSize )
        {
            // Note, page size is guaranteed to be a larger power of 2 than table->valueAlignment, so in this case the alignment to page size is enough.
            uint64_t paddedStoreSize = ( table->largeValueStoreSize + ( table->pageSize - 1 ) ) & ~( table->pageSize - 1 );

            result =
                bitable_wf_write( table->largeValueFile.file,
                                  table->largeValueFile.buffer,
                                  (uint32_t)( paddedStoreSize - table->largeValueStoreSize ) );

            if ( result != BR_SUCCESS )
            {
                return result;
            }

            table->largeValueStoreSize = paddedStoreSize;
        }
        else if ( paddedStoreOffset > table->largeValueStoreSize )
        {
            // we have to pad at the end of the large value store before we append.
            result = bitable_wf_write( table->largeValueFile.file, table->largeValueFile.buffer, (uint32_t)( paddedStoreOffset - table->largeValueStoreSize ) );

            table->largeValueStoreSize = paddedStoreOffset;

            if ( result != BR_SUCCESS )
            {
                return result;
            }
        }

        result = bitable_wf_write( table->largeValueFile.file, data->data, data->size );

        if ( result != BR_SUCCESS )
        {
            return result;
        }

        *(uint64_t*)( leafFile->buffer + ( table->pageSize - newRightSize ) ) = table->largeValueStoreSize;

        table->largeValueStoreSize += data->size;
    }

    {
        uint16_t           keyOffset      = table->pageSize - newKeyAllocation;
        BitableLeafIndice* itemIndice     = leafLevel->itemIndices + *leafLevel->itemCount;

        if ( key->size > 0 )
        {
            void* keyDestination = leafFile->buffer + keyOffset;

            memcpy( keyDestination, key->data, key->size );
        }

        itemIndice->dataSize   = data->size;
        itemIndice->itemOffset = keyOffset;
        itemIndice->keySize    = (uint16_t)key->size; // this is safe as maximum keysize is guaranteed to fit in a u16.

        leafLevel->leftSize  = newLeftSize;
        leafLevel->rightSize = newRightSize;
    }

    *leafLevel->itemCount += 1;
    ++table->itemCount;

    return BR_SUCCESS;
}

BitableResult bitable_writable_stats( const BitableWritable* table, BitableStats* stats )
{
    stats->depth               = table->depth;
    stats->itemCount           = table->itemCount;
    stats->keyAlignment        = table->keyAlignment;
    stats->largeValueStoreSize = table->largeValueStoreSize;
    stats->leafPages           = table->leafLevel.leafPageCount;
    stats->pageSize            = table->pageSize;
    stats->valueAlignment      = table->valueAlignment;

    return BR_SUCCESS;
}

static BitableResult finish_writes( BitableWritable* table, BitableCompletionOptions options )
{
    BitableResult result;
    BranchLevel* branchLevel;

    for ( branchLevel = table->branchLevels; branchLevel < table->branchLevels + table->depth && branchLevel->bufferedFile.file != NULL; ++branchLevel )
    {
        BufferedFile* branchFile = &branchLevel->bufferedFile;

        result = bitable_wf_write( branchFile->file, branchFile->buffer, table->pageSize );

        if ( result != BR_SUCCESS )
        {
            return result;
        }

        if ( ( options & BCO_DURABLE ) == BCO_DURABLE )
        {
            result = bitable_wf_sync( branchFile->file );

            if ( result != BR_SUCCESS )
            {
                return result;
            }
        }
    }

    if ( table->largeValueFile.file != NULL && ( options & BCO_DURABLE ) == BCO_DURABLE )
    {
        result = bitable_wf_sync( table->largeValueFile.file );

        if ( result != BR_SUCCESS )
        {
            return result;
        }
    }

    {
        LeafLevel*    leafLevel = &table->leafLevel;
        BufferedFile* leafFile  = &leafLevel->bufferedFile;

        if ( leafLevel->itemCount > 0 )
        {
            result = bitable_wf_write( leafFile->file, leafFile->buffer, table->pageSize );

            if ( result != BR_SUCCESS )
            {
                return result;
            }
        }

        if ( ( options & BCO_DURABLE ) == BCO_DURABLE )
        {
            result = bitable_wf_sync( leafFile->file );

            if ( result != BR_SUCCESS )
            {
                return result;
            }
        }

        {
            BitableHeader header;

            header.headerMarker        = BITABLE_HEADER_MARKER;
            header.itemCount           = table->itemCount;
            header.largeValueStoreSize = table->largeValueStoreSize;
            header.depth               = table->depth;
            header.keyAlignment        = table->keyAlignment;
            header.valueAlignment      = table->valueAlignment;
            header.pageSize            = table->pageSize;
            header.leafPages           = leafLevel->leafPageCount;
            header.checksum            = bitable_header_checksum( &header );

            result = bitable_wf_seek( leafFile->file, 0 );

            if ( result != BR_SUCCESS )
            {
                return result;
            }

            result = bitable_wf_write( leafFile->file, &header, sizeof( BitableHeader ) );

            if ( result != BR_SUCCESS )
            {
                return result;
            }

            if ( ( options & BCO_DURABLE ) == BCO_DURABLE )
            {
                result = bitable_wf_sync( leafFile->file );
            }

            if ( result != BR_SUCCESS )
            {
                return result;
            }
        }
    }

    return BR_SUCCESS;
}

BitableResult bitable_write_close( BitableWritable* table, BitableCompletionOptions options )
{
    BitableResult result = BR_SUCCESS;

    if ( ( options & BCO_DISCARD ) != BCO_DISCARD )
    {
        result = finish_writes( table, options );
    }

    cleanup_writable( table );

    return result;
}

void bitable_write_free( BitableWritable* table )
{
    cleanup_writable( table );
    free( table );
}