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
  * @brief Writable file IO support.
  */
#ifndef WRITABLE_FILE_H__
#define WRITABLE_FILE_H__
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bitablecommon.h"

/** OS specific structure for the handle information for a writable file.
  */
typedef struct BitableWritableFile BitableWritableFile;

/** Create a file for writing, allocating the handle information
  * @param [out] file Allocated open file handling for writing - should be closed with bitable_wf_close if this open function is successful. No null check performed.
  * @param path The file path to open, should be in UTF8 encoding. Does not null check.
  * @return A return code indicating either success, or the reason for failure.
  */
BITABLE_API BitableResult bitable_wf_create( BitableWritableFile** file, const char* path );

/** Seek to a position in a previously opened file relative the beginning.
  * @param file The file to seek in. Does not null check.
  * @param position The position to seek to.
  * @return A return code indicating either success, or the reason for failure.
  */
BITABLE_API BitableResult bitable_wf_seek( BitableWritableFile* file, int64_t position );

/** Write data to the current file point for a file.
  * @param file The file to write to. Does not null check.
  * @param data The data to write. Does not null check.
  * @param size The amount of data to write in bytes.
  * @return A return code indicating either success, or the reason for failure.
  */
BITABLE_API BitableResult bitable_wf_write( BitableWritableFile* file, const void* data, uint32_t size );

/** Sync a file to disk, including its metadata.
  * @param file The file to sync. Does not null check.
  * @return A return code indicating either success, or the reason for failure.
  */
BITABLE_API BitableResult bitable_wf_sync( BitableWritableFile* file );

/** Close a previously opened file.
  * @param file The file to close. Does not null check.
  * @return A return code indicating either success, or the reason for failure.
  */
BITABLE_API BitableResult bitable_wf_close( BitableWritableFile* file );

#ifdef __cplusplus
}
#endif 

#endif // -- WRITABLE_FILE_H__