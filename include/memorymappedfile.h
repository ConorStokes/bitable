#ifndef MEMORY_MAPPED_FILE_H__
#define MEMORY_MAPPED_FILE_H__
#pragma once

#include "bitablecommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BitableMemoryMappedFileHandle BitableMemoryMappedFileHandle;

/** Memory mapped file
 *  Includes the handle, size of the mapping and the address of the mapping.
 *  The handle is OS specific, but the size and address are standardstandard.
 */
typedef struct BitableMemoryMappedFile 
{

    /** The OS specific handle
     */
    BitableMemoryMappedFileHandle* handle;

    /** Size in bytes of the memory mapped file allocation. 
     */
    size_t size;

    /** The memory addres of the mapping.
     */
    void* address;

} BitableMemoryMappedFile;

/** Opens a read-only memory mapped file.
 *  Will map the entire file. 
 * @param [out] memoryMappedFile This will be populated with the memory mapped file details and should be passed to close when done. Does not null check.
 * @param path This is the path of the file that will be mapped into memory. This is expected to be in a UTF8 encoding. Does not null check.
 * @param openFlags This is the options for opening the memory mapped file.
 * @return A return code indicating the success of the operation, or a value indicating the kind of error that occured otherwise.
 */
BITABLE_API BitableResult bitable_mmf_open( BitableMemoryMappedFile* memoryMappedFile, const char* path, BitableReadOpenFlags openFlags );

/** Closes a read-only memory mapped file that has been opened by bitable_mmf_open 
*
* @param memoryMappedFile This will be populated with the memory mapped file details and should be passed to close when done. Does not null check.
* @return A return code indicating the success of the operation, or a value indicating the kind of error that occured otherwise.
*/
BITABLE_API BitableResult bitable_mmf_close( BitableMemoryMappedFile* memoryMappedFile );

#ifdef __cplusplus
}
#endif 

#endif