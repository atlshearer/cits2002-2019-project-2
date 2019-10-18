#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

int SIFS_readheader(FILE* volume, SIFS_VOLUME_HEADER* vol_header)
{
    if (fseek(volume, 0L, SEEK_SET) != 0) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fread(vol_header, sizeof(SIFS_VOLUME_HEADER), 1, volume);
    
    return 0;
}

int SIFS_readdirblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_DIRBLOCK* dir_block)
{
    
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + vol_header.nblocks + block_id * vol_header.blocksize, SEEK_SET) != 0) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fread(dir_block, sizeof(SIFS_DIRBLOCK), 1, volume);
    
    return 0;
}

int SIFS_readfileblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_FILEBLOCK* file_block)
{
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + vol_header.nblocks + block_id * vol_header.blocksize, SEEK_SET) != 0) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fread(file_block, sizeof(SIFS_FILEBLOCK), 1, volume);
    
    return 0;
}

int SIFS_readdata(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, void* data, size_t nbytes)
{
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + vol_header.nblocks + block_id * vol_header.blocksize, SEEK_SET)) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fread(data, nbytes, 1, volume);
    
    return 0;
}

int SIFS_readblocktype(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_BIT* block_type)
{
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + block_id, SEEK_SET)) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fread(block_type, sizeof(SIFS_BIT), 1, volume);
    
    return 0;
}