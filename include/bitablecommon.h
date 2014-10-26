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
#define BITABLE_API
#endif

#define BITABLE_MAX_KEY_SIZE 768
#define BITABLE_MIN_PAGE_SIZE 2048
#define BITABLE_MAX_PAGE_SIZE 65536
#define BITABLE_MAX_ALIGNMENT 512
#define BITABLE_MAX_BRANCH_LEVELS 64

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

    const void* data;
    int32_t size; // maximum value size is 2^31

} BitableValue;

/** The file paths for different files used by a bitable
 */
typedef struct BitablePaths
{

    char* leafPath;
    char* largeValuePath;
    char* branchPaths[ BITABLE_MAX_BRANCH_LEVELS ];

} BitablePaths;

/** Comparison function used for comparing two keys when searching a bitable.
  */
typedef int (BitableComparisonFunction)(const BitableValue* left, const BitableValue* right);

/** Take a paths object and populate it with the potential sub-paths of a bitable. 
  * Will allocate memory to populate the paths. Does not read from the file system at all,
  * builds the maximum number of paths deterministically.
  * @param [out] paths The paths to be populated. Does not null check.
  * @param basePath The main path for the bitable. Does not null check.
  */
BITABLE_API void bitable_build_paths( BitablePaths* paths, const char* basePath );

/** Free the memory for the file paths allocated by bitable_build_paths
  * @param paths The paths to be freed. Does not null check.
  */
BITABLE_API void bitable_free_paths( BitablePaths* paths );

#ifdef __cplusplus
}
#endif 

#endif // -- BITABLE_COMMON_H__