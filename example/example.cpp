#include "bitablewrite.h"
#include "bitableread.h"
#include <stdio.h>

// All keys will in the simple value table will be less than this.
static const int SIMPLE_TABLE_UPPER = 1024 * 1024;

// All keys in the large value table will be less than this.
static const int LARGE_VALUE_UPPER  = 4096;

static int key_compare( const BitableValue* left, const BitableValue* right ) 
{ 
    return *(int32_t*)left->data - *(int32_t*)right->data; 
} 

// Scope owner for allocating a bitable writable.
class WritableOwner
{
public:

    BitableWritable* writable;

    WritableOwner() { writable = bitable_write_allocate(); }

    ~WritableOwner() { bitable_write_free( writable ); }

private:

    WritableOwner( const WritableOwner& );

    WritableOwner& operator=( const WritableOwner& );
};

// Scope owner for allocate a bitable readable.
class ReadableOwner
{
public:

    BitableReadable* readable;

    ReadableOwner() { readable = bitable_read_allocate(); }

    ~ReadableOwner() { bitable_read_free( readable ); }

private:

    ReadableOwner( const ReadableOwner& );

    ReadableOwner& operator=( const ReadableOwner& );
};

static bool write_simple_table( BitableWritable* writable )
{
    // create the table file
    BitableResult result = bitable_write_create( writable, "example.btl", 4096, 4, 4 );

    if ( result != BR_SUCCESS )
    {
        printf( "Failed creating example.btl - %d\n", result );
        return false;
    }

    printf( "Appending keys...\n" );

    // iterate through and append key/value pairs (where, where), with a step of 2, to leave some free key spaces to do upper/lower bound query tests later.
    for ( int32_t where = 0; where < SIMPLE_TABLE_UPPER; where += 2 )
    {
        BitableValue key;

        key.data = &where;
        key.size = sizeof( int32_t );

        result = bitable_append( writable, &key, &key );

        if ( result != BR_SUCCESS )
        {
            printf( "Failed appending key - %d\n", result );
            return false;
        }
    }

    printf( "Flushing and closing files...\n" );

    // Close the bitable, using the BCO_NONE option to write it out without the durability guarantee.
    result = bitable_write_close( writable, BCO_NONE );
    
    if ( result != BR_SUCCESS )
    {
        printf( "Failed to close bitable - %d\n", result );
        return false;
    }

    return true;
}

static void read_simple_sequential( BitableReadable* readable )
{
    printf( "Doing sequential scan...\n" );

    BitableResult result = BR_SUCCESS;
    BitableCursor cursor;
    int counter = 0;

    // Iterate through each item from the first, until we run out of items or there is an error.
    for ( result = bitable_first( &cursor, readable ); result == BR_SUCCESS; result = bitable_next( &cursor, readable ), counter += 2 )
    {
        BitableValue key;
        BitableValue value;

        // Read the key value pair
        bitable_key_value_pair( &cursor, readable, &key, &value );

        // Some checks to make sure the values are okay.
        if ( *(int32_t*)key.data != *(int32_t*)value.data || *(int32_t*)key.data != counter )
        {
            printf( "Value read from value or key read from table doesn't match expected value - %d\n", counter );

            return;
        }
    }

    // make sure we didn't have an error other than hitting the end of the sequence.
    if ( result != BR_END_OF_SEQUENCE )
    {
        printf( "An error occured iterating through the sequence - %d\n", result );

        return;
    }

    return;
}

static void read_simple_exact( BitableReadable* readable )
{
    printf( "Doing exact key searches...\n" );

    // iterate through the keys and do find operations, doing exact matches.
    for ( int32_t where = 0; where < SIMPLE_TABLE_UPPER; where += 2 )
    {       
        BitableCursor cursor;
        BitableValue key;
        BitableValue value;

        key.data = &where;
        key.size = sizeof( int32_t );

        BitableResult result = bitable_find( &cursor, readable, &key, BFO_EXACT );

        // If this isn't BR_SUCCESS, it's typically because the key could not be found.
        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't find key %d - %d\n", where, result );

            return;
        }

        // Read the key and the value from the cursor.
        result = bitable_key_value_pair( &cursor, readable, &key, &value );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );

            return;
        }

        // Check the values.
        if ( *(int32_t*)key.data != *(int32_t*)value.data || *(int32_t*)key.data != where )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );

            return;
        }
    }    
}

static void read_simple_upper( BitableReadable* readable )
{
    printf( "Doing upper bound key searches...\n" );
    
    // Iterate through doing upper bound key searches in the holes in the key space, this should find the previous key.
    for ( int32_t where = 1; where < SIMPLE_TABLE_UPPER; where += 2 )
    {
        BitableCursor cursor;
        BitableValue key;
        BitableValue value;

        key.data = &where;
        key.size = sizeof( int32_t );

        BitableResult result = bitable_find( &cursor, readable, &key, BFO_UPPER );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't find key %d - %d\n", where, result );
            return;
        }

        result = bitable_key_value_pair( &cursor, readable, &key, &value );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );
            return;
        }

        if ( *(int32_t*)key.data != *(int32_t*)value.data || *(int32_t*)key.data != where - 1 )
        {
            printf( "Unexpected key or value - %d\n", where );
            return;
        }
    }
}

static void read_simple_lower( BitableReadable* readable )
{
    printf( "Doing lower bound key searches...\n" );

    // Iterate through doing lower bound key searches in the holes in the key space, this should find the next key.
    for ( int32_t where = 1; where < SIMPLE_TABLE_UPPER - 1; where += 2 )
    {
        BitableCursor cursor;
        BitableValue key;
        BitableValue value;

        key.data = &where;
        key.size = sizeof( int32_t );

        BitableResult result = bitable_find( &cursor, readable, &key, BFO_LOWER );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't find key %d - %d\n", where, result );
            return;
        }

        result = bitable_key_value_pair( &cursor, readable, &key, &value );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );
            return;
        }

        if ( *(int32_t*)key.data != *(int32_t*)value.data || *(int32_t*)key.data != where + 1 )
        {
            printf( "Unexpected key or value %d - %d\n", where, result );
            return;
        }
    }
}

static void read_simple_table( BitableReadable* readable )
{
    printf( "Opening simple table for reading\n" );

    BitableResult result = bitable_read_open( readable, "example.btl", BRO_NONE, key_compare );

    if ( result != BR_SUCCESS )
    {
        printf( "Failed to open bitable for reading - %d\n", result );
        return;
    }
    
    read_simple_sequential( readable );
    read_simple_exact( readable );
    read_simple_upper( readable );
    read_simple_lower( readable );

    BitableStats stats;

    bitable_readable_stats( readable, &stats );

    result = bitable_read_close( readable );

    if ( result != BR_SUCCESS )
    {
        printf( "Couldn't close readable bitable\n" );
    }

    BitablePaths paths;

    printf( "Deleting simple table files...\n" );

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
}

bool write_large_value_table( BitableWritable* writable )
{
    printf( "Writing large value table\n" );

    BitableResult result = bitable_write_create( writable, "example2.btl", 4096, 4, 4 );

    if ( result != BR_SUCCESS )
    {
        printf( "Failed creating example2.btl - %d\n", result );
        return false;
    }

    printf( "Appending keys...\n" );

    uint32_t* valueBuffer = new uint32_t[ LARGE_VALUE_UPPER ];

    for ( int32_t where = 0; where < LARGE_VALUE_UPPER; ++where )
    {
        BitableValue key;
        BitableValue value;

        valueBuffer[ where ] = where;

        value.data = valueBuffer;
        value.size = sizeof( uint32_t ) * ( where + 1 );

        key.data = &where;
        key.size = sizeof( int32_t );

        result = bitable_append( writable, &key, &value );

        if ( result != BR_SUCCESS )
        {
            delete[] valueBuffer;
            printf( "Failed appending key - %d %d\n", where, result );
            return false;
        }
    }

    delete[] valueBuffer;
    result = bitable_write_close( writable, BCO_NONE );

    if ( result != BR_SUCCESS )
    {
        printf( "Failed closing large value table - %d\n", result );
        return false;
    }

    return true;
}

static void read_large_sequential( BitableReadable* readable )
{
    printf( "Doing sequential scan...\n" );
    
    BitableResult result = BR_SUCCESS;
    BitableCursor cursor;
    int counter = 0;

    for ( result = bitable_first( &cursor, readable ); result == BR_SUCCESS; result = bitable_next( &cursor, readable ), counter += 1 )
    {
        BitableValue key;
        BitableValue value;

        bitable_key_value_pair( &cursor, readable, &key, &value );

        if ( *(int32_t*)key.data != counter )
        {
            printf( "Key read from table doesn't match expected value - %d\n", counter );

            return;
        }
        
        int32_t* valuePointer = (int32_t*)value.data;

        if ( value.size != ( counter + 1 ) * sizeof( int32_t ) )
        {
            printf( "Value size doesn't match expected value - (Key) %d (size) %d\n", counter, value.size );
            return;
        }

        for ( int valueCursor = 0; valueCursor < counter; ++valueCursor )
        {
            if ( valuePointer[ valueCursor ] != valueCursor )
            {
                printf( "Value is not expected - %d %d\n", counter, valueCursor );
                return;
            }
        }
    }

    if ( result != BR_SUCCESS && result != BR_END_OF_SEQUENCE )
    {
        printf( "An error occured iterating through the sequence - %d\n", result );

        return;
    }

    return;
}

static void read_large_exact( BitableReadable* readable )
{
    printf( "Doing exact key searches...\n" );

    for ( int32_t where = 0; where < LARGE_VALUE_UPPER; ++where )
    {
        BitableCursor cursor;
        BitableValue key;
        BitableValue value;

        key.data = &where;
        key.size = sizeof( int32_t );

        BitableResult result = bitable_find( &cursor, readable, &key, BFO_EXACT );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't find key %d - %d\n", where, result );

            return;
        }

        result = bitable_key_value_pair( &cursor, readable, &key, &value );

        if ( result != BR_SUCCESS )
        {
            printf( "Couldn't read values at key %d - %d\n", where, result );

            return;
        }

        if ( *(int32_t*)key.data != where )
        {
            printf( "Key unexpected %d - %d\n", where, result );

            return;
        }
        
        if ( value.size != ( where + 1 ) * sizeof( int32_t ) )
        {
            printf( "Value size doesn't match expected value - (Key) %d (size) %d\n", where, value.size );
            return;
        }

        int32_t* valuePointer = (int32_t*)value.data;

        for ( int valueCursor = 0; valueCursor < where; ++valueCursor )
        {
            if ( valuePointer[ valueCursor ] != valueCursor )
            {
                printf( "Value is not expected - %d %d\n", where, valueCursor );
                return;
            }
        }
    }
}

static void read_large_value_table( BitableReadable* readable )
{
    printf( "Opening large table for reading\n" );

    BitableResult result = bitable_read_open( readable, "example2.btl", BRO_NONE, key_compare );

    if ( result != BR_SUCCESS )
    {
        printf( "Failed to open bitable for reading - %d\n", result );
        return;
    }

    read_large_sequential( readable );
    read_large_exact( readable );

    BitableStats stats;

    bitable_readable_stats( readable, &stats );

    result = bitable_read_close( readable );

    if ( result != BR_SUCCESS )
    {
        printf( "Couldn't close readable bitable\n" );
    }

    BitablePaths paths;

    printf( "Deleting large table files...\n" );

    bitable_build_paths( &paths, "example2.btl" );

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
}

int main( int argc, char* argv[] )
{
    printf( "Creating table to append...\n" );

    WritableOwner writableOwner;
    ReadableOwner readableOwner;
    BitableWritable* writable = writableOwner.writable;
    BitableReadable* readable = readableOwner.readable;
    
    if ( !write_simple_table( writable ) )
    {
        return 0;
    }

    read_simple_table( readable );

    if ( !write_large_value_table( writable ) )
    {
        return 0;
    }

    read_large_value_table( readable );

    printf( "Done.\n" );

    return 0;
}

