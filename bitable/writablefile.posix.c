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

#include "writablefile.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <memory.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

typedef struct BitableWritableFile
{

    int fileDescriptor;

} BitableWritableFile;

static void cleanup_wf( BitableWritableFile* file )
{
    if ( file != NULL )
    {
        if ( file->fileDescriptor != -1 )
        {
            close( file->fileDescriptor );
        }

        free( file );
    }
}

BitableResult bitable_wf_create( BitableWritableFile** file, const char* path )
{
    BitableWritableFile* fileResult = calloc( 1, sizeof( BitableWritableFile ) );

    fileResult->fileDescriptor = open( path, O_CREAT | O_TRUNC | O_WRONLY, 0666 );

    if ( fileResult->fileDescriptor == -1 )
    {
        cleanup_wf( fileResult );
        return BR_FILE_OPEN_FAILED;
    }

    *file = fileResult;

    return BR_SUCCESS;

}

BitableResult bitable_wf_seek( BitableWritableFile* file, int64_t position )
{
    if ( lseek( file->fileDescriptor, (off_t)position, SEEK_SET ) == (off_t)-1 )
    {
        return BR_FILE_OPERATION_FAILED;
    }

    return BR_SUCCESS;
}

BitableResult bitable_wf_write( BitableWritableFile* file, const void* data, uint32_t size )
{
    if ( write( file->fileDescriptor, data, size ) == -1 )
    {
        return BR_FILE_OPERATION_FAILED;
    }

    return BR_SUCCESS;
}

BitableResult bitable_wf_sync( BitableWritableFile* file )
{
    if ( fsync( file->fileDescriptor ) == -1 )
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
