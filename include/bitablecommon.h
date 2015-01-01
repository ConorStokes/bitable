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
  * @brief Common definitions for the bitable interface, shared between reading and writing
  */
#ifndef BITABLE_COMMON_H__
#define BITABLE_COMMON_H__
#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Used to define DLL exports/imports across platforms. Define BITABLE_DLL if using bitable in dll mode*/
#if defined BITABLE_DLL
#if defined _WIN32 || defined __CYGWIN__
#if defined BITABLE_DLL_EXPORT
#define BITABLE_API __declspec(dllexport)
#else
#define BITABLE_API __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4 || defined __clang__
#define BITABLE_API __attribute__ ((visibility ("default")))
#endif
#endif
#else 

/** The API prefix macro for the bitable exported functions
  */
#define BITABLE_API
#endif

/** The maximum allowed keysize in bytes. You may not have keys larger than this.
  */
#define BITABLE_MAX_KEY_SIZE 768

/** The minimum allowed page size in bytes. You may not specify pages smaller than this.
  */
#define BITABLE_MIN_PAGE_SIZE 2048

/** The maximum page size allowed in bytes. Note, the limit is 64k because internal offsets are 16bit unsigned.
  */
#define BITABLE_MAX_PAGE_SIZE 65536

/** The maximum alignment that can be used for keys and values.
  */
#define BITABLE_MAX_ALIGNMENT 512

/** The maximum number of branch levels used before indexing the bitable.
  */
#define BITABLE_MAX_BRANCH_LEVELS 32

/** Return code used by bitable functions to indicate success, error conditions etc.
 */
typedef enum BitableResult
{
    /** The operation completed successfully.
      */
    BR_SUCCESS                  = 0,

    /** The cursor or find operation resulted in going out of the bounds of the bitable.
      */
    BR_END_OF_SEQUENCE          = 1,

    /** Failed to open the file.
      */
    BR_FILE_OPEN_FAILED         = 2,
    
    /** An OS level file operation failed.
      */
    BR_FILE_OPERATION_FAILED    = 3,

    /** The bitable file is too large - this usually occurs when a file is too large to be mapped into the address space (e.g. on 32bit platforms)
      */
    BR_FILE_TOO_LARGE           = 4,

    /** A file path provided was invalid.
      */
    BR_BAD_PATH                 = 5,

    /** The bitable passed in is already in an open state and can't be opened again.
      */
    BR_ALREADY_OPEN             = 6,

    /** The bitable file is too small (smaller than a page, or too small to read the header)
      */
    BR_FILE_TOO_SMALL           = 7,

    /** The bitable header is corrupt - either the header identifier is wrong or the checksum is invalid.
      */
    BR_HEADER_CORRUPT           = 8,

    /** An exact match (or similar) search was performed and the particular key wasn't found.
      */
    BR_KEY_NOT_FOUND            = 9,

    /** An operation was attempted on a cursor that wasn't in the valid range.
     */
    BR_INVALID_CURSOR_LOCATION  = 10,

    /** The bitable is larger than the maximum allowed tree depth.
      */
    BR_MAXIMUM_TABLE_TREE_DEPTH = 11,

    /** A key that has been passed in is too large (larger than BITABLE_MAX_KEY_SIZE) or has a negative size
      */
    BR_KEY_INVALID            = 12,

    /** The page size is too small (less than BITABLE_MIN_PAGE_SIZE), too large (greater than BITABLE_MAX_PAGE_SIZE) or not a power of two.
      */
    BR_PAGESIZE_INVALID         = 13,

    /** A value passed in for key or value data alignment is too large (greater than BITABLE_MAX_ALIGNMENT) or not a power of two.
      */
    BR_ALIGNMENT_INVALID        = 14

} BitableResult;

/** Flags used for opening bitable files indicating the intended access patterns.
  */
typedef enum BitableReadOpenFlags
{

    /** No options.
      */
    BRO_NONE = 0,

    /** Access will be mostly random
      */
    BRO_RANDOM = 1,

    /** Access will be mostly sequential.
      */
    BRO_SEQUENTIAL = 2

} BitableReadOpenFlags;

/** Represents a value used for keys/value data by bitable. Basically a pointer to a data buffer and a size.
  */
typedef struct BitableValue
{
    /** A pointer to the data associated with the value.
      */
    const void* data;

    /** The size (in bytes) of the value. Maximum value is 2^31.
      */
    int32_t size; 

} BitableValue;

/** The file paths for different files used by a bitable
 */
typedef struct BitablePaths
{
    /** The path of the main data file
      */
    char* leafPath;

    /** The path of the large value file
      */
    char* largeValuePath;

    /** The path of branch level files, up to the maximum branch level.
      */
    char* branchPaths[ BITABLE_MAX_BRANCH_LEVELS ];

} BitablePaths;

/** Statistics and information from the readable bitable header.
*/
typedef struct BitableStats
{

    /** The number of items in the table.
     */
    uint64_t itemCount;

    /** The number of leaf pages in the table.
     */
    uint64_t leafPages;

    /** The number of bytes the large value store takes up.
     */
    uint64_t largeValueStoreSize;

    /** The depth (number of branch levels, excluding the leaf file).
     */
    uint32_t depth;

    /** The alignment used for keys in the file in bytes - will be a power of 2 - key addresses will be aligned with this.
     */
    uint32_t keyAlignment;

    /** The aligment used for values in the file - will be a power of 2 - value addresses will be aligned with this.
     */
    uint32_t valueAlignment;

    /** The number of bytes used for the pages used for this bitable. Will be a power of 2.
     */
    uint32_t pageSize;

} BitableStats;

/** The type for a comparison function used for comparing two keys when searching a bitable.
  */
typedef int (BitableComparisonFunction)(const BitableValue* left, const BitableValue* right);

/** Take a paths object and populate it with the potential sub-paths of a bitable. 
  * Will allocate memory to populate the paths. Does not read from the file system at all,
  * builds the maximum number of paths deterministically.
  * Note, uses malloc to allocate paths.
  * @param [out] paths The paths to be populated. Should not be null.
  * @param basePath The main path for the bitable. Should not be null. Null terminated UTF8 is expected.
  */
BITABLE_API void bitable_build_paths( BitablePaths* paths, const char* basePath );

/** Free the memory for the file paths allocated by bitable_build_paths
  * @param paths The paths to be freed. Should not be null.
  */
BITABLE_API void bitable_free_paths( BitablePaths* paths );

#ifdef __cplusplus
}
#endif 

#endif // -- BITABLE_COMMON_H__