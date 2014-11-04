/** @file
  * @brief Interface for reading from a bitable.
  */
#ifndef BITABLE_READ_H__
#define BITABLE_READ_H__
#pragma once

#include "bitablecommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A bitable that can be read from.
  */
typedef struct BitableReadable BitableReadable;

/** A cursor - represents a position in the bitable that contains a key value pair. 
  */
typedef struct BitableCursor
{

    /** The leaf page the cursor is located in.
      */
    uint64_t page;

    /** The individual item in the page the cursor is located at.
      */
    int32_t item;

} BitableCursor;

/** Operations that can be used with the find function.
  */
typedef enum BitableFindOperation
{
    /** Lower bound search - use this to find the item at the start of a range (inclusive). 
      */
    BFO_LOWER = 0,

    /** Upper bound search - use this to find the item at the end of a range (inclusive).
      */
    BFO_UPPER = 1,
    
    /** Exact match search - will return an error if the find operation doesn't result in an exact match. 
     */
    BFO_EXACT = 2

} BitableFindOperation;


/** Allocate a zeroed readable bitable, to be used with bitable_read_open (can be re-used multiple times, when a table is closed).
  * @return The allocated readable bitable.
  */
BITABLE_API BitableReadable* bitable_read_allocate();

/** Open a bitable from the file system for reading. 
  * Apart from bitable_read_allocate, this is the only read function that allocates memory directly (memory may be demand paged
  * for reading from the memory mapped files).
  * It is safe to open the same path multiple times with different BitableReadable instances, even in different processes. 
  * You should not try and re-use the same BitableReadable instance without closing it. 
  * Closing/opening with regards to the BitableReadable instance are not thread safe operations so they should only be done exclusively.
  * @param [out] table The previously allocated readable bitable to open into. Should not be null.
  * @param path The path to the bitable to open, in UTF8 encoding. Should not be null.
  * @param openFlags The flags to use for opening the bitable - note reading hints will be applied to leaf and large value files only, 
  * @param comparison A comparison function that will be used to compare keys for searching. This should match the sort order when the keys were appended. Should not be null.
  * @return BR_SUCCESS if the table could be successfully opened for reading, an error code otherwise (BR_ALREADY_OPEN, BR_FILE_OPERATION_FAILED, BR_FILE_TOO_SMALL, BR_HEADER_CORRUPT, BR_FILE_OPEN_FAILED, BR_FILE_TOO_LARGE).
  */
BITABLE_API BitableResult bitable_read_open( BitableReadable* table, const char* path, BitableReadOpenFlags openFlags, BitableComparisonFunction* comparison );

/** Close the file handles (etc) associated with a previously opened readable bitable. 
  * Note, this operation can be called on an already closed table. 
  * This does not free the memory associated with the BitableReadable, so it can be re-used.
  * Closing/opening with regards to the BitableReadable instance are not thread safe operations so they should only be done exclusively.
  * @param table The readable bitable to close. Should not be null.
  * @return BR_SUCCESS if the table could be successfully closed.
*/
BITABLE_API BitableResult bitable_read_close( BitableReadable* table );

/** Close the file handles (etc) associated with a previously opened readable bitable and frees the memory associated with it.
  * Note, this operation can be called on an already closed table to de-allocate it.
  * Closing/opening with regards to the BitableReadable instance are not thread safe operations so they should only be done exclusively.
  * @param table The readable bitable to close. Should not be null.
  */
BITABLE_API void bitable_read_free( BitableReadable* table );

/** Get the statistics associated with a particular bitable (including the number of items, depth, page size, key and value alignments etc).
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param table The open readable bitable to get the stats from. Should not be null.
  * @param [out] stats The stats from the table. Should not be null.
  * @return BR_SUCCESS if the stats could successfully be retrieved.
  */
BITABLE_API BitableResult bitable_readable_stats( const BitableReadable* table, BitableStats* stats );

/** Given a key and find operation, find a matching position in the bitable and populate the cursor with it.  
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param [out] cursor The cursor that will be populated with the find position. Should not be null.
  * @param table The open readable bitable to find the key in. Should not be null.
  * @param searchKey The key to search for. Should not be null.
  * @param operation The operation to use for searching.
  * @return BR_SUCCESS if the operation is successful. BR_KEY_NOT_FOUND is the operation is BFO_EXACT and the key doesn't exist in the bitable. BR_END_OF_SEQUENCE if the operation is an upper/lower bound and the bound is outside the bitable range.
*/
BITABLE_API BitableResult bitable_find( BitableCursor* cursor, const BitableReadable* table, const BitableValue* searchKey, BitableFindOperation operation );

/** Populate the cursor with the first position (beginning) of the bitable.
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param [out] cursor The cursor that will be populated with the first position. Should not be null.
  * @param table The open readable bitable to get the position of. Should not be null.
  * @return BR_SUCCESS if the operation is successful. BR_END_OF_SEQUENCE if the bitable is empty.
*/
BITABLE_API BitableResult bitable_first( BitableCursor* cursor, const BitableReadable* table );

/** Populate the cursor with the last position (at the end) of the bitable.
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param [out] cursor The cursor that will be populated with the last position. Should not be null.
  * @param table The open readable bitable to get the position of. Should not be null.
  * @return BR_SUCCESS if the operation is successful. BR_END_OF_SEQUENCE if the bitable is empty.
  */
BITABLE_API BitableResult bitable_last( BitableCursor* cursor, const BitableReadable* table );

/** Populate the cursor with the next position from its current value.
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param [in,out] cursor The cursor that will be incremented to the next position. Should not be null.
  * @param table The open readable bitable to perform the operation with. Should not be null.
  * @return BR_SUCCESS if the operation is fdsuccessful. BR_END_OF_SEQUENCE if the next position is beyond the end of the sequence.
  */
BITABLE_API BitableResult bitable_next( BitableCursor* cursor, const BitableReadable* table );

/** Populate the cursor with the previous position from its current value.
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param [in,out] cursor The cursor that will be decremented to the previous position. Should not be null.
  * @param table The open readable bitable to perform the operation with. Should not be null.
  * @return BR_SUCCESS if the operation is successful. BR_END_OF_SEQUENCE if the next position is beyond the end of the sequence.
  */
BITABLE_API BitableResult bitable_previous( BitableCursor* cursor, const BitableReadable* table );

/** Read the key at a particular cursor position from the bitable.
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param cursor The cursor position that will be read from. Should not be null.
  * @param table The open readable bitable to read from. Should not be null.
  * @param [out] key The key to read out. Should not be null.
  * @return BR_SUCCESS if the operation is successful. BR_INVALID_CURSOR_LOCATION if the cursor position isn't valid.
  */
BITABLE_API BitableResult bitable_key( const BitableCursor* cursor, const BitableReadable* table, BitableValue* key );

/** Read the value at a particular cursor position from the bitable.
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param cursor The cursor position that will be read from. Should not be null.
  * @param table The open readable bitable to read from. Should not be null.
  * @param [out] value The key value read out.
  * @return BR_SUCCESS if the operation is successful. BR_INVALID_CURSOR_LOCATION if the cursor position isn't valid.
  */
BITABLE_API BitableResult bitable_value( const BitableCursor* cursor, const BitableReadable* table, BitableValue* value );

/** Read both the key and value value at a particular cursor position from the bitable.
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param cursor The cursor position that will be read from. Should not be null.
  * @param table The open readable bitable to read from. Should not be null.
  * @param [out] key The key to read out. Should not be null.
  * @param [out] value The key value read out. Should not be null.
  * @return BR_SUCCESS if the operation is successful. BR_INVALID_CURSOR_LOCATION if the cursor position isn't valid.
  */
BITABLE_API BitableResult bitable_key_value_pair( const BitableCursor* cursor, const BitableReadable* table, BitableValue* key, BitableValue* value );

/** Read the indice (0 based) at a particular cursor position.
  * The indice number of key value pairs before the one at the cursor.
  * This method is thread-safe on an open table (but the table must be open for the duration of the call).
  * @param cursor The cursor position that will be read from. Should not be null.
  * @param table The open readable bitable to read from. Should not be null.
  * @param [out] indice The indice to read out. Should not be null.
  * @return BR_SUCCESS if the operation is successful. BR_INVALID_CURSOR_LOCATION if the cursor position isn't valid.
  */
BITABLE_API BitableResult bitable_indice( const BitableCursor* cursor, const BitableReadable* table, uint64_t* indice );

#ifdef __cplusplus
}
#endif

#endif // -- BITABLE_READ_H__
