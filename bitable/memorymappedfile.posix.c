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
