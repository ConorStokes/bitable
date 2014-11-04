#include "memorymappedfile.h"
#include "bitableread.h"
#include "bitableshared.h"
#include <memory.h>
#include <assert.h>

typedef struct BitableReadable
{

    BitableHeader* header;
    BitableMemoryMappedFile leafFile;
    BitableMemoryMappedFile branchFiles[ BITABLE_MAX_BRANCH_LEVELS ];
    BitableMemoryMappedFile largeValueFile;
    BitableComparisonFunction* comparison;
    BitablePaths paths;

} BitableReadable;

/** Cleans up a readable bitable and closes the files associated with it.
  * @param table The table to cleanup.
  */
static void cleanup_table( BitableReadable* table )
{
    uint32_t where;

    bitable_mmf_close( &table->leafFile );
    bitable_mmf_close( &table->largeValueFile );

    for ( where = 0; where < BITABLE_MAX_BRANCH_LEVELS; ++where )
    {
        bitable_mmf_close( &table->branchFiles[ where ] );
    }

    bitable_free_paths( &table->paths );

    memset( table, 0, sizeof( BitableReadable ) );
}

BitableReadable* bitable_read_allocate()
{
    BitableReadable* result = calloc( 1, sizeof( BitableReadable ) );

    return result;
}

BitableResult bitable_read_open( BitableReadable* table, const char* path, BitableReadOpenFlags openFlags, BitableComparisonFunction* comparison )
{
    BitableResult result = BR_SUCCESS;
    uint32_t where;

    table->comparison = comparison;

    if ( table->header != NULL )
    {
        return BR_ALREADY_OPEN;
    }

    bitable_build_paths( &table->paths, path );

    result = bitable_mmf_open( &table->leafFile, table->paths.leafPath, openFlags );

    if ( result != BR_SUCCESS )
    {
        cleanup_table( table );
        return result;
    }

    if ( table->leafFile.size < sizeof( BitableHeader ) )
    {
        cleanup_table( table );
        return BR_FILE_TOO_SMALL;
    }

    table->header = table->leafFile.address;

    if ( bitable_header_checksum( table->header ) != table->header->checksum )
    {
        cleanup_table( table );
        return BR_HEADER_CORRUPT;
    }

    if ( table->header->largeValueStoreSize > 0 )
    {
        result = bitable_mmf_open( &table->largeValueFile, table->paths.largeValuePath, openFlags );

        if ( result != BR_SUCCESS )
        {
            cleanup_table( table );
            return result;
        }
    }

    for ( where = 0; where < table->header->depth; ++where )
    {
        result = bitable_mmf_open( &table->branchFiles[ where ], table->paths.branchPaths[ where ], BRO_RANDOM );

        if ( result != BR_SUCCESS )
        {
            cleanup_table( table );
            return result;
        }
    }

    return BR_SUCCESS;
}

void bitable_read_free( BitableReadable* table )
{
    cleanup_table( table );
    free( table );
}

BitableResult bitable_read_close( BitableReadable* table )
{
    cleanup_table( table );
    return BR_SUCCESS;
}

BitableResult bitable_readable_stats( const BitableReadable* table, BitableStats* stats )
{
    /* TODO - error handling here */

    const BitableHeader* header = table->header;

    stats->depth               = header->depth;
    stats->itemCount           = header->itemCount;
    stats->keyAlignment        = header->keyAlignment;
    stats->valueAlignment      = header->valueAlignment;
    stats->pageSize            = header->pageSize;
    stats->largeValueStoreSize = header->largeValueStoreSize;
    stats->leafPages           = header->leafPages;

    return BR_SUCCESS;
}

BitableResult bitable_first( BitableCursor* cursor, const BitableReadable* table )
{
    if ( table->header->itemCount == 0 )
    {
        return BR_END_OF_SEQUENCE;
    }

    cursor->page = 0;
    cursor->item = 0;

    return BR_SUCCESS;
}

BitableResult bitable_last( BitableCursor* cursor, const BitableReadable* table )
{
    if ( table->header->itemCount == 0 )
    {
        return BR_END_OF_SEQUENCE;
    }

    cursor->page = table->header->leafPages - 1;

    {
        const uint64_t* page = (const uint64_t*)table->leafFile.address + ( table->header->pageSize * ( cursor->page + 1 ) );

        int32_t itemCount = *(const int32_t*)( page + 1 );

        cursor->item = itemCount - 1;
    }

    return BR_SUCCESS;
}

BitableResult bitable_find( BitableCursor* cursor, const BitableReadable* table, const BitableValue* searchKey, BitableFindOperation operation )
{
    BitableComparisonFunction* comparison = table->comparison;
    uint64_t                   childPage  = 0;
    int level;

    // iterate through the branch levels
    for ( level = ( (int)table->header->depth ) - 1; level >= 0; --level )
    {
        const void*                node             = (const uint8_t*)table->branchFiles[ level ].address + ( table->header->pageSize * childPage );
        uint64_t                   baseChild        = *(const uint64_t*)node;
        int                        childCount       = *(const uint16_t*)( (const uint8_t*)node + sizeof( uint64_t ) );
        const BitableBranchIndice* nodeIndex        = (const BitableBranchIndice*)( (const uint8_t*)node + sizeof( uint64_t ) + sizeof( uint16_t ) );
        int                        low              = 0;
        int                        high             = childCount - 2;
        int                        best             = -1;
        int                        comparisonResult = -1;

        // Upper bound search with termination on equals -
        // will find the item equal to first below the key.
        // If no best item is found, then the child for the first node (which doesn't have a key in the array) is taken.
        while ( low <= high && comparisonResult != 0 )
        {
            int                        mid     = low + ( ( high - low ) / 2 );
            const BitableBranchIndice* indice  = nodeIndex + mid;
            BitableValue               readKey;

            readKey.data = (const uint8_t*)node + indice->itemOffset;
            readKey.size = indice->keySize;

            comparisonResult = comparison( &readKey, searchKey );

            if ( comparisonResult <= 0 )
            {
                best = mid;
                low  = mid + 1;
            }
            else
            {
                high = mid - 1;
            }
        }

        childPage = best >= 0 ? ( baseChild + best + 1 ) : baseChild;
    }

    BitableResult result = BR_SUCCESS;

    cursor->page = childPage;

    // as opposed to the exact or lower search above, we do a lower bound search below
    {
        const void*              node           = (const uint8_t*)table->leafFile.address + ( table->header->pageSize * ( childPage + 1 ) );
        int                      itemCount      = *(const int32_t*)((const uint8_t*)node + sizeof( uint64_t ) );
        const BitableLeafIndice* leafIndex      = (const BitableLeafIndice*)( (const uint8_t*)node + sizeof( uint64_t ) + sizeof( int32_t ) );
        int                      low            = 0;
        int                      high           = itemCount - 1;
        int                      best           = -1;
        int                      bestComparison = 1;

        while ( high >= low && bestComparison != 0 )
        {
            BitableValue readKey;
            int mid = low + ( ( high - low ) / 2 );
            int comparisonResult;

            readKey.data = (const uint8_t*)node + ( leafIndex[ mid ].itemOffset );
            readKey.size = leafIndex[ mid ].keySize;

            comparisonResult = comparison( &readKey, searchKey );

            if ( comparisonResult >= 0 )
            {
                bestComparison = comparisonResult;
                best           = mid;
                high           = mid - 1;
            }
            else
            {
                low = mid + 1;
            }
        }

        if ( best >= 0 )
        {
            cursor->item = best;

            if ( bestComparison != 0 )
            {
                switch ( operation )
                {
                case BFO_UPPER:

                    result = bitable_previous( cursor, table );
                    break;

                case BFO_EXACT:

                    result = ( bestComparison == 0 ) ? BR_SUCCESS : BR_KEY_NOT_FOUND;
                    break;

                default:

                    break;
                }
            }
        }
        else
        {
            cursor->item = itemCount - 1;

            switch ( operation )
            {
            case BFO_LOWER:

                result = bitable_next( cursor, table );
                break;

            case BFO_EXACT:

                result = BR_KEY_NOT_FOUND;
                break;

            default:

                break;
            }
        }
    }

    return result;
}

BitableResult bitable_next( BitableCursor* cursor, const BitableReadable* table )
{
    if ( cursor->page >= table->header->leafPages || cursor->item < 0 )
    {
        return BR_END_OF_SEQUENCE;
    }

    {
        const void* page      = (const uint8_t*)table->leafFile.address + ( table->header->pageSize * ( cursor->page + 1 ) );
        int32_t     itemCount = *(const int32_t*)( (const uint8_t*)page + +sizeof( uint64_t ) );
        int32_t     nextItem  = cursor->item + 1;

        if ( nextItem < itemCount )
        {
            cursor->item = nextItem;
        }
        else if ( cursor->page + 1 >= table->header->leafPages )
        {
            return BR_END_OF_SEQUENCE;
        }
        else
        {
            ++cursor->page;
            cursor->item = 0;
        }
    }

    return BR_SUCCESS;
}

BitableResult bitable_previous( BitableCursor* cursor, const BitableReadable* table )
{
    if ( cursor->page >= table->header->leafPages || ( cursor->page == 0 && cursor->item == 0 ) )
    {
        return BR_END_OF_SEQUENCE;
    }

    {
        const void* page          = (const uint8_t*)table->leafFile.address + ( table->header->pageSize * ( cursor->page + 1 ) );
        int32_t     itemCount     = *(const int32_t*)( (const uint8_t*)page + +sizeof( uint64_t ) );
        int32_t     previousItem  = cursor->item - 1;

        if ( previousItem < itemCount && previousItem >= 0 )
        {
            cursor->item = previousItem;
        }
        else if ( previousItem < 0 )
        {
            --cursor->page;

            {
                const void* previousPage      = (const uint8_t*)table->leafFile.address + ( table->header->pageSize * ( cursor->page + 1 ) );
                int32_t     previousItemCount = *(const int32_t*)( (const uint8_t*)previousPage + sizeof( uint64_t ) );

                cursor->item = previousItemCount - 1;
            }
        }
        else
        {
            return BR_END_OF_SEQUENCE;
        }
    }

    return BR_SUCCESS;
}

BitableResult bitable_key( const BitableCursor* cursor, const BitableReadable* table, BitableValue* key )
{
    if ( cursor->page >= table->header->leafPages || cursor->item < 0 )
    {
        return BR_INVALID_CURSOR_LOCATION;
    }

    {
        const void*              page      = (const uint8_t*)table->leafFile.address + ( table->header->pageSize * ( cursor->page + 1 ) );
        int32_t                  itemCount = *(const int32_t*)( (const uint8_t*)page + sizeof( uint64_t ) );
        const BitableLeafIndice* leafIndex = (const BitableLeafIndice*)( (const uint8_t*)page + sizeof( uint64_t ) + sizeof( int32_t ) );

        if ( cursor->item < 0 || cursor->item >= itemCount )
        {
            return BR_INVALID_CURSOR_LOCATION;
        }

        {
            const BitableLeafIndice* itemIndice = leafIndex + cursor->item;

            key->size = itemIndice->keySize;
            key->data = (const uint8_t*)page + itemIndice->itemOffset;
        }
    }

    return BR_SUCCESS;
}


BitableResult bitable_value( const BitableCursor* cursor, const BitableReadable* table, BitableValue* value )
{
    if ( cursor->page >= table->header->leafPages || cursor->item < 0 )
    {
        return BR_INVALID_CURSOR_LOCATION;
    }

    {
        const void*              page      = (const uint8_t*)table->leafFile.address + ( table->header->pageSize * ( cursor->page + 1 ) );
        int32_t                  itemCount = *(const int32_t*)( (const uint8_t*)page + sizeof( uint64_t ) );
        const BitableLeafIndice* leafIndex = (const BitableLeafIndice*)( (const uint8_t*)page + sizeof( uint64_t ) + sizeof( int32_t ) );

        if ( cursor->item < 0 || cursor->item >= itemCount )
        {
            return BR_INVALID_CURSOR_LOCATION;
        }

        {
            const BitableLeafIndice* itemIndice     = leafIndex + cursor->item;
            const int32_t            dataFromRight  = table->header->pageSize - itemIndice->itemOffset;

            value->size = itemIndice->dataSize;

            if ( value->size <= BITABLE_MAX_KEY_SIZE )
            {
                const uint32_t paddedOffset = table->header->pageSize - ( ( dataFromRight + itemIndice->dataSize + ( table->header->valueAlignment - 1 ) ) & ~( table->header->valueAlignment - 1 ) );
                const void*    dataAddress  = (const uint8_t*)page + paddedOffset;

                value->data = value->size > 0 ? dataAddress : NULL;
            }
            else
            {
                const uint32_t paddedOffset     = table->header->pageSize - ( ( dataFromRight + sizeof( uint64_t ) + ( sizeof( uint64_t ) - 1 ) ) & ~( sizeof( uint64_t ) - 1 ) );
                const void*    dataAddress      = (const uint8_t*)page + paddedOffset;
                size_t         largeValueOffset = (size_t)*(const uint64_t*)dataAddress;

                value->data = (const uint8_t*)table->largeValueFile.address + largeValueOffset;

                assert( table->largeValueFile.size >= largeValueOffset + value->size );
            }
        }
    }

    return BR_SUCCESS;
}

BitableResult bitable_key_value_pair( const BitableCursor* cursor, const BitableReadable* table, BitableValue* key, BitableValue* value )
{
    if ( cursor->page >= table->header->leafPages || cursor->item < 0 )
    {
        return BR_INVALID_CURSOR_LOCATION;
    }

    {
        const void*              page      = (const uint8_t*)table->leafFile.address + ( table->header->pageSize * ( cursor->page + 1 ) );
        int32_t                  itemCount = *(const int32_t*)( (const uint8_t*)page + sizeof( uint64_t ) );
        const BitableLeafIndice* leafIndex = (const BitableLeafIndice*)( (const uint8_t*)page + sizeof( uint64_t ) + sizeof( int32_t ) );

        if ( cursor->item < 0 || cursor->item >= itemCount )
        {
            return BR_INVALID_CURSOR_LOCATION;
        }

        {
            const BitableLeafIndice* itemIndice     = leafIndex + cursor->item;
            const uint32_t           dataFromRight  = table->header->pageSize - itemIndice->itemOffset;

            key->size   = itemIndice->keySize;
            key->data   = (const uint8_t*)page + itemIndice->itemOffset;
            value->size = itemIndice->dataSize;

            if ( value->size <= BITABLE_MAX_KEY_SIZE )
            {
                const uint32_t paddedOffset = table->header->pageSize - ( ( dataFromRight + itemIndice->dataSize + ( table->header->valueAlignment - 1 ) ) & ~( table->header->valueAlignment - 1 ) );
                const void*    dataAddress  = (const uint8_t*)page + paddedOffset;

                value->data = value->size > 0 ? dataAddress : NULL;
            }
            else
            {
                const uint32_t paddedOffset     = table->header->pageSize - ( ( dataFromRight + sizeof( uint64_t ) + ( sizeof( uint64_t ) - 1 ) ) & ~( sizeof( uint64_t ) - 1 ) );
                const void*    dataAddress      = (const uint8_t*)page + paddedOffset;
                size_t         largeValueOffset = (size_t)*(const uint64_t*)dataAddress;

                value->data = (const uint8_t*)table->largeValueFile.address + largeValueOffset;

                assert( table->largeValueFile.size >= largeValueOffset + value->size );
            }
        }
    }

    return BR_SUCCESS;
}


BitableResult bitable_indice( const BitableCursor* cursor, const BitableReadable* table, uint64_t* indice )
{
    if ( cursor->page >= table->header->leafPages || cursor->item < 0 )
    {
        return BR_INVALID_CURSOR_LOCATION;
    }

    {
        const void* page      = (const uint8_t*)table->leafFile.address + ( table->header->pageSize * ( cursor->page + 1 ) );
        uint64_t    baseIndex = *(const uint64_t*)page;
        int32_t     itemCount = *(const int32_t*)( (const uint8_t*)page + +sizeof( uint64_t ) );

        if ( cursor->item >= itemCount )
        {
            return BR_INVALID_CURSOR_LOCATION;
        }

        *indice = baseIndex + cursor->item;
    }

    return BR_SUCCESS;
}
