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
  * @brief Read only memory mapped file support.
  */
#ifndef MEMORY_MAPPED_FILE_H__
#define MEMORY_MAPPED_FILE_H__
#pragma once

#include "bitablecommon.h"

#ifdef __cplusplus
extern "C" {
#endif

/** OS specific handle for a memory mapped file.
  */
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