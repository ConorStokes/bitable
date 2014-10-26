#include "bitablewrite.h"
#include "bitableread.h"
#include <stdio.h>

static int key_compare( const BitableValue* left, const BitableValue* right ) 
{ 
    return *(int32_t*)left->data - *(int32_t*)right->data; 
} 

int main( int argc, char* argv[] )
{
    BitableWritable* writable = bitable_write_allocate();
    BitableResult    result   = bitable_write_create( writable, "example.btl", 4096, 4, 4 );

    if ( result != BR_SUCCESS )
    {
        printf( "Failed creating example.btl - %d\n", result );
        return 0;
    }

    printf( "Appending keys...\n" );

    for ( int32_t where = 0; where < 1024 * 1024; where += 2 )
    {
        BitableValue key;

        key.data = &where;
        key.size = sizeof( int32_t );

        result = bitable_append( writable, &key, &key );

        if ( result != BR_SUCCESS )
        {
            printf( "Failed appending key - %d\n", result );
            bitable_write_free( writable );
            return 0;
        }
    }

    printf( "Flushing and closing files...\n" );

    result = bitable_write_close( writable, BCO_NONE );

    bitable_write_free( writable );

    if ( result != BR_SUCCESS )
    {
        printf( "Failed to close bitable - %d\n", result );
        return 0;
    }
    
    printf( "Opening for read...\n" );

    BitableReadable* readable = bitable_read_allocate();

    result = bitable_read_open( readable, "example.btl", BRO_NONE, key_compare );

    if ( result != BR_SUCCESS )
    {
        printf( "Failed to open bitable for reading - %d\n", result );
        return 0;
    }

    BitableCursor cursor;

    int counter = 0;

    printf( "Iterating over all key/value pairs sequentially...\n" );

    for ( result = bitable_first( &cursor, readable ); result == BR_SUCCESS; result = bitable_next( &cursor, readable ), counter += 2 )
    {
        BitableValue key;
        BitableValue value;

        bitable_key_value_pair( &cursor, readable, &key, &value );

        if ( *(int32_t*)key.data != *(int32_t*)value.data || *(int32_t*)key.data != counter )
        {
            printf( "Value read from value or key read from table doesn't match expected value - %d\n", counter );
            bitable_read_free( readable );
            return 0;
        }
    }

    if ( result != BR_SUCCESS && result != BR_END_OF_SEQUENCE )
    {
        printf( "An error occured iterating through the sequence - %d\n", result );
        return 0;
    }

    printf( "Doing exact key searches...\n" );

    for ( int32_t where = 0; where < 1024 * 1024; where += 2 )
    {
        BitableValue key;
        BitableValue value;

        key.data = &where;
        key.size = sizeof( int32_t );

        result = bitable_find( &cursor, readable, &key, BFO_EXACT );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't find key %d - %d\n", where, result );
            return 0;
        }

        result = bitable_key_value_pair( &cursor, readable, &key, &value );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );
            return 0;
        }

        if ( *(int32_t*)key.data != *(int32_t*)value.data || *(int32_t*)key.data != where )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );
            return 0;
        }
    }

    printf( "Doing upper bound key searches...\n" );

    for ( int32_t where = 1; where < 1024 * 1024; where += 2 )
    {
        BitableValue key;
        BitableValue value;

        key.data = &where;
        key.size = sizeof( int32_t );

        result = bitable_find( &cursor, readable, &key, BFO_UPPER );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't find key %d - %d\n", where, result );
            return 0;
        }

        result = bitable_key_value_pair( &cursor, readable, &key, &value );
        
        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );
            return 0;
        }

        if ( *(int32_t*)key.data != *(int32_t*)value.data || *(int32_t*)key.data != where - 1 )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );
            return 0;
        }
    }

    printf( "Doing lower bound key searches...\n" );

    for ( int32_t where = 1; where < 1024 * 1024 - 1; where += 2 )
    {
        BitableValue key;
        BitableValue value;

        key.data = &where;
        key.size = sizeof( int32_t );

        result = bitable_find( &cursor, readable, &key, BFO_LOWER );
        
        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't find key %d - %d\n", where, result );
            return 0;
        }

        result = bitable_key_value_pair( &cursor, readable, &key, &value );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );
            return 0;
        }

        if ( *(int32_t*)key.data != *(int32_t*)value.data || *(int32_t*)key.data != where + 1 )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );
            return 0;
        }
    }

    BitableReadableStats stats;

    bitable_stats( readable, &stats );

    bitable_read_free( readable );

    BitablePaths paths;

    printf( "Deleting files...\n" );

    bitable_build_paths( &paths, "example.btl" );

    for ( uint32_t where = 0; where < stats.depth; ++where )
    {
        remove( paths.branchPaths[ where ] );
    }

    if ( stats.largeValueStoreSize > 0 )
    {
        remove( paths.largeValuePath );
    }

    remove( paths.leafPath );

    bitable_free_paths( &paths );

    return 0;
}

