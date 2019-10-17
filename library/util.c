//
//  util.c
//  
//
//  Created by Alexander Shearer on 17/10/19.
//

#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

#include <errno.h>


// reader helpers

int SIFS_getheader(FILE* volume, SIFS_VOLUME_HEADER* vol_header)
{
    if (fseek(volume, 0L, SEEK_SET) != 0) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fread(vol_header, sizeof(SIFS_VOLUME_HEADER), 1, volume);
    
    return 0;
}

int SIFS_getdirblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_DIRBLOCK* dir_block)
{
    
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + vol_header.nblocks + block_id * vol_header.blocksize, SEEK_SET) != 0) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fread(dir_block, sizeof(SIFS_DIRBLOCK), 1, volume);
    
    return 0;
}

int SIFS_getfileblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_FILEBLOCK* file_block)
{
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + vol_header.nblocks + block_id * vol_header.blocksize, SEEK_SET) != 0) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fread(file_block, sizeof(SIFS_FILEBLOCK), 1, volume);
    
    return 0;
}

int SIFS_getblocktype(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_BIT* block_type)
{
    if (fseek(volume, sizeof(SIFS_VOLUME_HEADER) + block_id, SEEK_SET)) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    
    fread(block_type, sizeof(SIFS_BIT), 1, volume);
    
    return 0;
}



// writer helpers

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

int SIFS_writedata(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_FILEBLOCK* data, size_t nbytes)
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





