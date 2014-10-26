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
