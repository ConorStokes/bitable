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
