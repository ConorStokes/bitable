/** @file
  * @brief Interface for writing to a bitable.
  */
#ifndef BITABLE_WRITE_H__
#define BITABLE_WRITE_H__
#pragma once

#include "bitablecommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Options for completing (flushing) the bitable out to disk as it is closed.
  */
typedef enum BitableCompletionOptions
{
    /** Bitable will be flushed out to the file system, with the header written last, however
      * no syncs will be done so write order to storage is not guaranteed. Use this option if it's not important
      * to be durable following a crash/power failure. 
      */
    BCO_NONE     = 0,

    /** Bitable will be flushed out to file system and syncs done in the correct order
      * such that completion is atomic and the rest of the bitable will be on storage
      * before the final header is written out. Headers are checksum'd to guarantee that
      * they are complete/correct.
      */
    BCO_DURABLE  = 1,

    /** Don't perform any completion writes, this bitable will not be used.
      */
    BCO_DISCARD  = 2

} BitableCompletionOptions;

/** A bitable that can be written to.
  */
typedef struct BitableWritable BitableWritable;

/** Allocate a zeroed writable bitable, to be used with bitable_write_create (can be re-used multiple times).
  * @return The allocated writable bitable.
*/
BITABLE_API BitableWritable* bitable_write_allocate();

/** Create an empty bitable for writing.
  * @param [out] table A writable bitable allocated with bitable_write_allocate that will be initialised with the parameters for the table.
  * @param path The path (UTF8 encoding) to create the bitable. This should be the main leaf/data file name.
  * @param pageSize The size of the page to use. Should be greater or equal to BITABLE_MIN_PAGE_SIZE and less than or equal to BITABLE_MAX_PAGE_SIZE. Should be a power of 2. Recommended to use a memory page size on your target platform.
  * @param keyAlignment The alignment that will be used for starting address of keys stored in the table. Needs to be greater than 0, less than BITABLE_MAX_ALIGNMENT and a power of 2. 
  * @param dataAlignment The alignment that will be used for starting address of data value stored in the table. Needs to be greater than 0, less than BITABLE_MAX_ALIGNMENT and a power of 2.
  * @return BR_SUCCESS if the table is successfully created. BR_ALREADY_OPEN if the table is already open, BR_PAGESIZE_INVALID if pageSize is not a valid value, BR_ALIGNMENT_INVALID if keyAlignment or dataAlignment are invalid. BR_FILE_OPEN_FAILED, BR_BAD_PATH or BR_FILE_OPERATION_FAILED if a file operation means the file can not be created.
  */
BITABLE_API BitableResult bitable_write_create( BitableWritable* table, const char* path, uint16_t pageSize, uint16_t keyAlignment, uint16_t dataAlignment );

/** Append a key value pair to the bitable. 
  * Keys and pairs are always appended in key sorted order. 
  * Duplicate keys are not currently supported.
  * It is the responsibility of the user to order the keys in the order matching the comparison function they will provide for reading. 
  * @param table A writable bitable created with bitable_write_create for the key/value pair to be appended to.
  * @param key The key of the key value pair to append. The key size needs to be less than BITABLE_MAX_KEY_SIZE
  * @param data The value data of the key value pair to append,
  * @return BR_SUCCESS if the table is successfully created. BR_KEY_INVALID if the key is not valid. BR_MAXIMUM_TABLE_TREE_DEPTH if the tree reaches its maximum depth. BR_FILE_OPERATION_FAILED or BR_FILE_OPEN_FAILED if a file operation fails.
*/
BITABLE_API BitableResult bitable_append( BitableWritable* table, const BitableValue* key, const BitableValue* data );

/** Close a writable bitable created with bitable_write_create. 
  * Does not free the memory associated with the writable bitable (that allocated by bitable_write_allocate).
  * Optionally complete or discard the remaining writes required for the table.
  * @param table The writable bitable to close.
  * @param options The options for completing (or discarding) the table on close.
  * @return BR_SUCCESS if the table is successfully completed/closed. BR_FILE_OPERATION_FAILED if file operations fail and prevent the completion of the bitable writes.
  */
BITABLE_API BitableResult bitable_write_close( BitableWritable* table, BitableCompletionOptions options );

/** Free the allocated bitable. If the bitable has not been closed, the bitable is closed with the BCO_DISCARD option.
  * @param table The writable bitable to free
  */
BITABLE_API void bitable_write_free( BitableWritable* table );

#ifdef __cplusplus
}
#endif 

#endif // -- BITABLE_WRITE_H__