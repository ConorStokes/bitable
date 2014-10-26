#include "memorymappedfile.h"
#include <memory.h>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

typedef struct BitableMemoryMappedFileHandle
{

    HANDLE fileHandle;

} BitableMemoryMappedFileHandle;

static void cleanup_mmf( BitableMemoryMappedFile* memoryMappedFile )
{
    if ( memoryMappedFile->address != NULL )
    {
        UnmapViewOfFile( memoryMappedFile->address );
    }

    memoryMappedFile->address = NULL;

    if ( memoryMappedFile->handle != NULL )
    {
        if ( memoryMappedFile->handle->fileHandle != NULL && memoryMappedFile->handle->fileHandle != INVALID_HANDLE_VALUE )
        {
            CloseHandle( memoryMappedFile->handle->fileHandle );
        }

        free( memoryMappedFile->handle );
        memoryMappedFile->handle = NULL;
    }
}

BitableResult bitable_mmf_open( BitableMemoryMappedFile* memoryMappedFile, const char* path, BitableReadOpenFlags openFlags )
{
    DWORD         fileFlags          = FILE_ATTRIBUTE_NORMAL;
    int           widePathBufferSize = MultiByteToWideChar( CP_UTF8, 0, path, -1, NULL, 0 );
    LPWSTR        widePathBuffer     = NULL;
    HANDLE        fileMappingHandle  = NULL;
    LARGE_INTEGER fileSize;

    memset( memoryMappedFile, 0, sizeof( BitableMemoryMappedFile ) );

    memoryMappedFile->handle = calloc( 1, sizeof( BitableMemoryMappedFileHandle ) );

    if ( widePathBufferSize <= 0 )
    {
        cleanup_mmf( memoryMappedFile );
        return BR_BAD_PATH;
    }

    widePathBuffer = malloc( sizeof( WCHAR ) * widePathBufferSize );

    if ( MultiByteToWideChar( CP_UTF8, 0, path, -1, widePathBuffer, widePathBufferSize ) <= 0 )
    {
        free( widePathBuffer );
        cleanup_mmf( memoryMappedFile );
        return BR_BAD_PATH;
    }

    memoryMappedFile->handle->fileHandle = NULL;

    if ( ( openFlags & BRO_RANDOM ) == BRO_RANDOM )
    {
        fileFlags = FILE_FLAG_RANDOM_ACCESS;
    }
    else if ( ( openFlags & BRO_SEQUENTIAL ) == BRO_SEQUENTIAL )
    {
        fileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    }

    memoryMappedFile->handle->fileHandle = CreateFileW( widePathBuffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, fileFlags, NULL );

    free( widePathBuffer );
    widePathBuffer = NULL;

    if ( memoryMappedFile->handle == INVALID_HANDLE_VALUE )
    {
        cleanup_mmf( memoryMappedFile );
        return BR_FILE_OPEN_FAILED;
    }

    if ( !GetFileSizeEx( memoryMappedFile->handle->fileHandle, &fileSize ) )
    {
        cleanup_mmf( memoryMappedFile );
        return BR_FILE_OPERATION_FAILED;
    }

    if ( (uint64_t)fileSize.QuadPart > ~(size_t)0 )
    {
        cleanup_mmf( memoryMappedFile );
        return BR_FILE_TOO_LARGE;
    }

    memoryMappedFile->size = (size_t)fileSize.QuadPart;

    fileMappingHandle = CreateFileMappingW( memoryMappedFile->handle->fileHandle, NULL, PAGE_READONLY, 0, 0, NULL );

    if ( fileMappingHandle == NULL )
    {
        cleanup_mmf( memoryMappedFile );
        return BR_FILE_OPERATION_FAILED;
    }

    memoryMappedFile->address = MapViewOfFile( fileMappingHandle, FILE_MAP_READ, 0, 0, memoryMappedFile->size );

    CloseHandle( fileMappingHandle );

    if ( memoryMappedFile->address == NULL )
    {
        cleanup_mmf( memoryMappedFile );
        return BR_FILE_OPERATION_FAILED;
    }

    return BR_SUCCESS;
}


BitableResult bitable_mmf_close( BitableMemoryMappedFile* memoryMappedFile )
{
    cleanup_mmf( memoryMappedFile );
    return BR_SUCCESS;
}