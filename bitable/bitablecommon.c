#define _CRT_SECURE_NO_WARNINGS 1

#include "bitablecommon.h"
#include <string.h>
#include <memory.h>
#include <stdio.h>

void bitable_build_paths( BitablePaths* paths, const char* basePath )
{
    size_t   basePathLength = strlen( basePath );
    uint32_t where;

    paths->leafPath = malloc( basePathLength + 1 );

    strcpy( paths->leafPath, basePath );

    paths->largeValuePath = malloc( basePathLength + 5 );

    strcpy( paths->largeValuePath, basePath );
    strcat( paths->largeValuePath, ".lvs" );

    for ( where = 0; where < BITABLE_MAX_BRANCH_LEVELS; ++where )
    {
        paths->branchPaths[ where ] = malloc( basePathLength + 5 );

        strcpy( paths->branchPaths[ where ], basePath );
        sprintf( paths->branchPaths[ where ] + basePathLength, ".%03d", where );
    }
}

void bitable_free_paths( BitablePaths* paths )
{
    uint32_t where;

    free( paths->leafPath );
    free( paths->largeValuePath );

    for ( where = 0; where < BITABLE_MAX_BRANCH_LEVELS; ++where )
    {
        free( paths->branchPaths[ where ] );
    }
}