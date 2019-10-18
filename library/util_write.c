//  CITS2002 Project 2 2019
//  Name(s):             Alexander Shearer, Thomas Kinsella
//  Student number(s):   22465777, 22177293

#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

int SIFS_writedirblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_DIRBLOCK* dir_block)
{
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + vol_header.nblocks + block_id * vol_header.blocksize, SEEK_SET) != 0) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fwrite(dir_block, sizeof(SIFS_DIRBLOCK), 1, volume);
    
    return 0;
}

int SIFS_writefileblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_FILEBLOCK* file_block)
{
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + vol_header.nblocks + block_id * vol_header.blocksize, SEEK_SET) != 0) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fwrite(file_block, sizeof(SIFS_FILEBLOCK), 1, volume);
    
    return 0;
}

int SIFS_writedata(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, void* data, size_t nbytes)
{
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + vol_header.nblocks + block_id * vol_header.blocksize, SEEK_SET)) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fwrite(data, nbytes, 1, volume);
    
    return 0;
}

int SIFS_writeblocktype(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_BIT* block_type, size_t nblocks)
{
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + block_id, SEEK_SET)) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    for (size_t i = 0; i < nblocks; i++) {
        fwrite(block_type, sizeof(SIFS_BIT), 1, volume);
    }
    
    return 0;
}
