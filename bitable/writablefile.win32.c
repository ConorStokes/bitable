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

#define WIN32_LEAN_AND_MEAN

#include "writablefile.h"
#include <memory.h>
#include <windows.h>

typedef struct BitableWritableFile
{

    HANDLE fileHandle;

} BitableWritableFile;

/** Clean up a writable file, closing the handle.
  * @param file The file to cleanup.
  */
static void cleanup_wf( BitableWritableFile* file )
{
    if ( file != NULL )
    {
        if ( file->fileHandle != NULL && file->fileHandle != INVALID_HANDLE_VALUE )
        {
            CloseHandle( file->fileHandle );
        }

        free( file );
    }
}

BitableResult bitable_wf_create( BitableWritableFile** file, const char* path )
{
    BitableWritableFile* fileResult         = calloc( 1, sizeof( BitableWritableFile ) );
    int                  widePathBufferSize = MultiByteToWideChar( CP_UTF8, 0, path, -1, NULL, 0 );
    LPWSTR               widePathBuffer     = NULL;

    if ( widePathBufferSize <= 0 )
    {
        cleanup_wf( fileResult );
        return BR_BAD_PATH;
    }

    widePathBuffer = malloc( sizeof( WCHAR ) * widePathBufferSize );

    if ( MultiByteToWideChar( CP_UTF8, 0, path, -1, widePathBuffer, widePathBufferSize ) <= 0 )
    {
        free( widePathBuffer );
        cleanup_wf( fileResult );
        return BR_BAD_PATH;
    }

    fileResult->fileHandle = CreateFileW( widePathBuffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    free( widePathBuffer );
    widePathBuffer = NULL;

    if ( fileResult->fileHandle == INVALID_HANDLE_VALUE )
    {
        cleanup_wf( fileResult );
        return BR_FILE_OPEN_FAILED;
    }

    *file = fileResult;

    return BR_SUCCESS;
}

BitableResult bitable_wf_seek( BitableWritableFile* file, int64_t position )
{
    LARGE_INTEGER convertedPosition;

    convertedPosition.QuadPart = position;

    if ( SetFilePointerEx( file->fileHandle, convertedPosition, NULL, FILE_BEGIN ) == FALSE )
    {
        return BR_FILE_OPERATION_FAILED;
    }

    return BR_SUCCESS;
}

BitableResult bitable_wf_write( BitableWritableFile* file, const void* data, uint32_t size )
{
    DWORD bytesWritten;

    if ( WriteFile( file->fileHandle, data, size, &bytesWritten, NULL ) == FALSE || bytesWritten != size )
    {
        return BR_FILE_OPERATION_FAILED;
    }

    return BR_SUCCESS;
}

BitableResult bitable_wf_sync( BitableWritableFile* file )
{
    if ( FlushFileBuffers( file->fileHandle ) == FALSE )
    {
        return BR_FILE_OPERATION_FAILED;
    }

    return BR_SUCCESS;
}

BitableResult bitable_wf_close( BitableWritableFile* file )
{
    cleanup_wf( file );
    return BR_SUCCESS;
}
