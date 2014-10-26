#include "bitableshared.h"

uint64_t bitable_header_checksum( const BitableHeader* header )
{
    uint64_t checksum = 0;

    checksum  = header->headerMarker;
    checksum *= 37;
    checksum += header->itemCount;
    checksum *= 37;
    checksum += header->largeValueStoreSize;
    checksum *= 37;
    checksum += header->depth;
    checksum *= 37;
    checksum += header->keyAlignment;
    checksum *= 37;
    checksum += header->valueAlignment;
    checksum *= 37;
    checksum += header->pageSize;
    checksum *= 37;
    checksum += header->leafPages;

    return checksum;
}