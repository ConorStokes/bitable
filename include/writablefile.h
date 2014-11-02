#ifndef WRITABLE_FILE_H__
#define WRITABLE_FILE_H__
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** OS specific structure for the handle information for a writable file.
  */
typedef struct BitableWritableFile BitableWritableFile;

#include "bitablecommon.h"

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
BITABLE_API BitableResult bitable_wf_close( BitableWritableFile* memoryMappedFile );

#ifdef __cplusplus
}
#endif 

#endif // -- WRITABLE_FILE_H__