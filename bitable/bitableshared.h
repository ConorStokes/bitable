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

#ifndef BITABLE_SHARED_H__
#define BITABLE_SHARED_H__
#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
    
/* The maximum number of branch levels for a bitable - should be less than 3 digits */
#define BITABLE_HEADER_MARKER 0xD47A682CF7E614BA

/** Header used at the front of the leaf page, should show it is a bitables leaf file, provide the needed stats to load other files,etc.
 */
typedef struct BitableHeader
{

    uint64_t headerMarker;
    uint64_t itemCount;
    uint64_t checksum;
    uint64_t largeValueStoreSize;
    uint32_t depth;
    uint32_t keyAlignment;
    uint32_t valueAlignment;
    uint32_t pageSize;
    uint64_t leafPages;

} BitableHeader;

/** Used to provide an index to individual key/value pairs in a leaf node in storage.
 */
typedef struct BitableLeafIndice
{

    uint32_t dataSize;
    uint16_t keySize;
    uint16_t itemOffset;

} BitableLeafIndice;

/** Used to provide an index to individual child nodes/keys in a branch node in storage.
 */
typedef struct BitableBranchIndice
{
    uint16_t keySize;
    uint16_t itemOffset;

} BitableBranchIndice;

/** Calculate the checksum for a header.
  * @param header The header to provide the checksum for.
  * @return The generated 64bit checksum for the header.
  */
uint64_t bitable_header_checksum( const BitableHeader* header );

#ifdef __cplusplus
}
#endif 

#endif // -- BITABLE_SHARED_H__
