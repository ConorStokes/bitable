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
