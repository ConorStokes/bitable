#define _FILE_OFFSET_BITS 64

#include "memorymappedfile.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <memory.h>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

typedef struct BitableMemoryMappedFileHandle
{

    int fileDescriptor;

} BitableMemoryMappedFileHandle;

static void cleanup_mmf( BitableMemoryMappedFile* memoryMappedFile )
{
    if ( memoryMappedFile->address != NULL && memoryMappedFile->address != MAP_FAILED )
    {
        munmap( memoryMappedFile->address, memoryMappedFile->size );
    }

    memoryMappedFile->address = NULL;

    if ( memoryMappedFile->handle != NULL )
    {
        if ( memoryMappedFile->handle->fileDescriptor != -1 )
        {
            close( memoryMappedFile->handle->fileDescriptor );
            memoryMappedFile->handle->fileDescriptor = -1;
        }

        free( memoryMappedFile->handle );
        memoryMappedFile->handle = NULL;
    }
}

BitableResult bitable_mmf_open( BitableMemoryMappedFile* memoryMappedFile, const char* path, BitableReadOpenFlags openFlags )
{
    memset( memoryMappedFile, 0, sizeof( BitableMemoryMappedFile ) );

    memoryMappedFile->handle = malloc( sizeof( BitableMemoryMappedFileHandle ) );

    {
        struct stat fileStats;
        int advice = POSIX_FADV_NORMAL;

        memoryMappedFile->handle->fileDescriptor = open( path, O_RDONLY );

        if ( memoryMappedFile->handle->fileDescriptor == -1 )
        {
            cleanup_mmf( memoryMappedFile );
            return BR_FILE_OPEN_FAILED;
        }
        
        switch ( openFlags )
        {
        case BRO_NONE:

            advice = POSIX_FADV_NORMAL;
            break;

        case BRO_RANDOM:

            advice = POSIX_FADV_RANDOM;
            break;

        case BRO_SEQUENTIAL:

            advice = POSIX_FADV_SEQUENTIAL;
            break;

        }

        if ( posix_fadvise( memoryMappedFile->handle->fileDescriptor, 0, 0, advice ) != 0 )
        {
            cleanup_mmf( memoryMappedFile );
            return BR_FILE_OPERATION_FAILED;
        }
        
        if ( fstat( memoryMappedFile->handle->fileDescriptor, &fileStats ) == -1 )
        {
            cleanup_mmf( memoryMappedFile );
            return BR_FILE_OPERATION_FAILED;
        }

        if ( fileStats.st_size > ((size_t)-1) )
        {
            cleanup_mmf( memoryMappedFile );
            return BR_FILE_TOO_LARGE;
        }

        memoryMappedFile->size    = (size_t)fileStats.st_size;
        memoryMappedFile->address = mmap( NULL, memoryMappedFile->size, PROT_READ, MAP_SHARED, memoryMappedFile->handle->fileDescriptor, 0 );

        if ( memoryMappedFile->address == MAP_FAILED )
        {
            cleanup_mmf( memoryMappedFile );
            return BR_FILE_OPERATION_FAILED;
        }
    }

    return BR_SUCCESS;
}


BitableResult bitable_mmf_close( BitableMemoryMappedFile* memoryMappedFile )
{
    cleanup_mmf( memoryMappedFile );
    return BR_SUCCESS;
}
