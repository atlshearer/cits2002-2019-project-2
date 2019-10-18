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


// traversal helpers

int SIFS_getfileblockid(FILE* volume, char **parsed_path, size_t path_depth, SIFS_VOLUME_HEADER vol_header, SIFS_BLOCKID* parent, SIFS_BLOCKID* target)
{
    /*
     #define    SIFS_EINVAL    1    // Invalid argument
     #define    SIFS_ENOENT    4    // No such file or directory entry
     #define    SIFS_ENOTVOL    6    // Not a volume
     #define    SIFS_ENOTDIR    7    // Not a directory
     #define    SIFS_ENOTFILE    8    // Not a file
     */
    
    SIFS_DIRBLOCK curr_dir;
    SIFS_DIRBLOCK next_dir;
    SIFS_BLOCKID curr_dir_id;
    SIFS_BLOCKID next_dir_id;
    
    SIFS_FILEBLOCK file_block;
    SIFS_BLOCKID file_block_id;
    
    // Read root block
    if (SIFS_readdirblock(volume, SIFS_ROOTDIR_BLOCKID, vol_header, &curr_dir)) {
        return 1;
    }
    
    // traverse down structure to target node
    for (size_t i = 0; i < path_depth; i++)
    {
        // check if curr dir contains next dir
        int found = 0;
        
        for (size_t j = 0; j < curr_dir.nentries && !found; j++)
        {
            next_dir_id = curr_dir.entries[j].blockID;
            
            SIFS_BIT block_type;
            if (SIFS_readblocktype(volume, next_dir_id, vol_header, &block_type) != 0) {
                return 1;
            }
            
            switch (block_type) {
                case SIFS_UNUSED:
                    SIFS_errno = SIFS_ENOTVOL; // If unused block is reference vol is malformed
                    return 1;
                    
                case SIFS_DIR:
                    SIFS_readdirblock(volume, next_dir_id, vol_header, &next_dir);
                    
                    if (strcmp(next_dir.name, parsed_path[i]) == 0)
                    {
                        if (i == path_depth - 1) {
                            SIFS_errno = SIFS_ENOTFILE;
                            return 1;
                        }
                        
                        found = 1;
                    }
                    break;
                    
                case SIFS_FILE:
                    if (i < path_depth - 1) {
                        SIFS_errno = SIFS_ENOTDIR;
                        return 1;
                    }
                    
                    SIFS_readfileblock(volume, next_dir_id, vol_header, &file_block);
                    
                    if (strcmp(file_block.filenames[curr_dir.entries[j].fileindex], parsed_path[i]) == 0) {
                        // Target found
                        found = 1;
                    }
                    
                    break;
                    
                case SIFS_DATABLOCK:
                    SIFS_errno = SIFS_ENOTVOL; // If datablock is reference vol is malformed
                    return 1;
                    
                default:
                    SIFS_errno = SIFS_ENOTVOL; // If blocktype is invalid vol is malformed
                    return 1;
            }
            
        }
        
        if (found && i < path_depth - 1) { // move into dir
            curr_dir = next_dir;
            curr_dir_id = next_dir_id;
        } else if (found) {
            // Target file in file_block
            file_block_id = next_dir_id;
        } else {
            SIFS_errno = SIFS_ENOENT;
            return 1;
        }
    }
    
    // curr_dir is parent
    // file_block is target
    
    *parent = curr_dir_id;
    *target = file_block_id;
    
    return 0;
}


// reader helpers

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





