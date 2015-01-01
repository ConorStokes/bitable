/*
Copyright (c) 2015, Conor Stokes
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
  * @param [out] table A writable bitable allocated with bitable_write_allocate that will be initialised with the parameters for the table. Should not be null.
  * @param path The path (UTF8 encoding) to create the bitable. This should be the main leaf/data file name. Should not be null.
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
  * Only directly allocates memory when a new branch level is added. May block performing I/O.
  * @param table A writable bitable created with bitable_write_create for the key/value pair to be appended to. Should not be null.
  * @param key The key of the key value pair to append. The key size needs to be less than BITABLE_MAX_KEY_SIZE. Should not be null.
  * @param data The value data of the key value pair to append. Should not be null.
  * @return BR_SUCCESS if the table is successfully created. BR_KEY_INVALID if the key is not valid. BR_MAXIMUM_TABLE_TREE_DEPTH if the tree reaches its maximum depth. BR_FILE_OPERATION_FAILED or BR_FILE_OPEN_FAILED if a file operation fails.
*/
BITABLE_API BitableResult bitable_append( BitableWritable* table, const BitableValue* key, const BitableValue* data );

/** Get the statistics associated with a particular bitable (including the number of items, depth, page size, key and value alignments etc).
  * @param table The open readable bitable to get the stats from. Should not be null.
  * @param [out] stats The stats from the table. Should not be null.
  * @return BR_SUCCESS if the stats could successfully be retrieved.
  */
BITABLE_API BitableResult bitable_writable_stats( const BitableWritable* table, BitableStats* stats );

/** Close a writable bitable created with bitable_write_create. 
  * Does not free the memory associated with the writable bitable (that allocated by bitable_write_allocate).
  * Optionally complete or discard the remaining writes required for the table.
  * @param table The writable bitable to close. Should not be null.
  * @param options The options for completing (or discarding) the table on close.
  * @return BR_SUCCESS if the table is successfully completed/closed. BR_FILE_OPERATION_FAILED if file operations fail and prevent the completion of the bitable writes.
  */
BITABLE_API BitableResult bitable_write_close( BitableWritable* table, BitableCompletionOptions options );

/** Free the allocated bitable. If the bitable has not been closed, the bitable is closed with the BCO_DISCARD option.
  * @param table The writable bitable to free. Should be allocated by bitable_write_allocate. Should not be null.
  */
BITABLE_API void bitable_write_free( BitableWritable* table );

#ifdef __cplusplus
}
#endif 

#endif // -- BITABLE_WRITE_H__