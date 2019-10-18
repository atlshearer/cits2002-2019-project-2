//  CITS2002 Project 2 2019
//  Name(s):             Alexander Shearer, Thomas Kinsella
//  Student number(s):   22465777, 22177293

#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

// Checks if name is a terminated string and check it does not contain '/'
static int validate_name(char name[SIFS_MAX_NAME_LENGTH])
{
    int has_null_byte = 0;
    
    
    for (size_t i = 0; i < SIFS_MAX_NAME_LENGTH; i++)
    {
        if (name[i] == 0)
        {
            if (i == 0) { // Empty name
                SIFS_errno = SIFS_ENOTVOL;
                return 1;
            }

            has_null_byte = 1;
        }

        if (name[i] == '/')
        {
            SIFS_errno = SIFS_ENOTVOL;
            return 1;
        }
    }
    
    if (!has_null_byte)
    {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }   

    return 0;
}

int SIFS_checkvolumeintegrity(FILE* volume, SIFS_VOLUME_HEADER vol_header)
{
    size_t total_size;
    SIFS_BIT block_type;

    SIFS_DIRBLOCK dir_block;
    SIFS_FILEBLOCK file_block;

    size_t blocks_checked;

    // Validate header
    if (vol_header.blocksize < SIFS_MIN_BLOCKSIZE)
    {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }

    // Validate file size
    total_size = sizeof(SIFS_VOLUME_HEADER) + 
                    sizeof(SIFS_BIT) * vol_header.nblocks + 
                    vol_header.blocksize * vol_header.nblocks;

    fseek(volume, 0, SEEK_END);
    if (ftell(volume) != total_size)
    {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }
    
    // Check validity of bitmap and blocks
    if (SIFS_readblocktype(volume, SIFS_ROOTDIR_BLOCKID, vol_header, &block_type) != 0)
    {
        return 1;
    }

    if (block_type != SIFS_DIR) {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }

    for (size_t i = 0; i < vol_header.nblocks; i++)
    {
        if (SIFS_readblocktype(volume, i, vol_header, &block_type) != 0)
        {
            return 1;
        }
        
        switch (block_type)
        {
        case SIFS_UNUSED:
            break;

        case SIFS_DIR:
            if (SIFS_readdirblock(volume, i, vol_header, &dir_block) != 0)
            {
                return 1;
            }

            if (i != SIFS_ROOTDIR_BLOCKID) {
                if (validate_name(dir_block.name) != 0)
                {
                    return 1;
                }                
            }

            if (dir_block.nentries > SIFS_MAX_ENTRIES) 
            {
                SIFS_errno = SIFS_ENOTVOL;
                return 1;
            }
            break;

        case SIFS_FILE:
            if (SIFS_readfileblock(volume, i, vol_header, &file_block) != 0)
            {
                return 1;
            }

            if (file_block.nfiles < 1 || file_block.nfiles > SIFS_MAX_ENTRIES)
            {
                SIFS_errno = SIFS_ENOTVOL;
                return 1;
            }

            for (size_t j = 0; j < file_block.nfiles; j++)
            {
                if (validate_name(file_block.filenames[j]) != 0) {
                    return 1;
                }
            }
            
            if (file_block.firstblockID < 0 || file_block.firstblockID >= vol_header.nblocks)
            {
                SIFS_errno = SIFS_ENOTVOL;
                return 1;
            }

            blocks_checked = 0;

            while (blocks_checked * vol_header.blocksize < file_block.length) {
                if (SIFS_readblocktype(volume, file_block.firstblockID + blocks_checked, vol_header, &block_type) != 0)
                {
                    return 1;
                }

                if (block_type != SIFS_DATABLOCK)
                {
                    SIFS_errno = SIFS_ENOTVOL;
                    return 1;
                }

                blocks_checked++;
            }

            break;

        case SIFS_DATABLOCK:
            break;
        
        default:
            SIFS_errno = SIFS_ENOTVOL;
            return 1;
        }
        
    }

    // Check validity of links
    for (size_t i = 0; i < vol_header.nblocks; i++)
    {
        if (SIFS_readblocktype(volume, i, vol_header, &block_type) != 0)
        {
            return 1;
        }
        
        if (block_type == SIFS_DIR)
        {
            if (SIFS_readdirblock(volume, i, vol_header, &dir_block) != 0)
            {
                return 1;
            }

            for (size_t i = 0; i < dir_block.nentries; i++)
            {
                if (dir_block.entries[i].blockID < 0 || dir_block.entries[i].blockID >= vol_header.nblocks)
                {
                    SIFS_errno = SIFS_ENOTVOL;
                    return 1;
                }

                if (SIFS_readblocktype(volume, dir_block.entries[i].blockID, vol_header, &block_type) != 0)
                {
                    return 1;
                }

                switch (block_type)
                {
                case SIFS_DIR:
                    break;

                case SIFS_FILE:
                    // Validate fileindex
                    if (SIFS_readfileblock(volume, dir_block.entries[i].blockID, vol_header, &file_block) != 0)
                    {
                        return 1;
                    }

                    if (dir_block.entries[i].fileindex < 0 || dir_block.entries[i].fileindex >= file_block.nfiles)
                    {
                        SIFS_errno = SIFS_ENOTVOL;
                        return 1;
                    }

                    break;
                
                default:
                    SIFS_errno = SIFS_ENOTVOL;
                    return 1;
                }
            }         
        }
    }

    return 0;
}

int SIFS_getfileblockid(FILE* volume, char **parsed_path, size_t path_depth, SIFS_VOLUME_HEADER vol_header, SIFS_BLOCKID* parent, SIFS_BLOCKID* target)
{    
    SIFS_DIRBLOCK curr_dir;
    SIFS_DIRBLOCK next_dir;
    SIFS_BLOCKID curr_dir_id = 0;
    SIFS_BLOCKID next_dir_id;
    
    SIFS_FILEBLOCK file_block;
    SIFS_BLOCKID file_block_id;
    
    if (path_depth < 1) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // Read root block
    if (SIFS_readdirblock(volume, SIFS_ROOTDIR_BLOCKID, vol_header, &curr_dir)) {
        return 1;
    }
        //printf("HERE\n");
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
                    if (SIFS_readdirblock(volume, next_dir_id, vol_header, &next_dir) != 0)
                    {
                        return 1;
                    }
                    
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
                    
                    if (SIFS_readfileblock(volume, next_dir_id, vol_header, &file_block) != 0)
                    {
                        return 1;
                    }
                    
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
        
    *parent = curr_dir_id;
    *target = file_block_id;
    
    return 0;
}

int SIFS_getdirblockid(FILE* volume, char **parsed_path, size_t path_depth, SIFS_VOLUME_HEADER vol_header, SIFS_BLOCKID* target)
{    
    SIFS_DIRBLOCK curr_dir;
    SIFS_DIRBLOCK next_dir;
    SIFS_BLOCKID curr_dir_id = 0;
    SIFS_BLOCKID next_dir_id;
    
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
                    if (SIFS_readdirblock(volume, next_dir_id, vol_header, &next_dir) != 0)
                    {
                        return 1;
                    }
                    
                    if (strcmp(next_dir.name, parsed_path[i]) == 0)
                    {                       
                        found = 1;
                    }
                    break;
                    
                case SIFS_FILE:
                    // Valid block_type, not target                    
                    break;
                    
                case SIFS_DATABLOCK:
                    SIFS_errno = SIFS_ENOTVOL; // If datablock is reference vol is malformed
                    return 1;
                    
                default:
                    SIFS_errno = SIFS_ENOTVOL; // If blocktype is invalid vol is malformed
                    return 1;
            }
        }
        
        if (found) { 
            // move into dir
            curr_dir = next_dir;
            curr_dir_id = next_dir_id;
        } else {
            SIFS_errno = SIFS_ENOENT;
            return 1;
        }
    }
    
    *target = curr_dir_id;
    
    return 0;
}

int SIFS_checkexists(FILE* volume, SIFS_DIRBLOCK parent, char* target_name, SIFS_VOLUME_HEADER vol_header) {
    SIFS_DIRBLOCK dir;
    SIFS_FILEBLOCK file;
    SIFS_BIT block_type;

    for (size_t i = 0; i < parent.nentries; i++)
    {
        if (SIFS_readblocktype(volume, parent.entries[i].blockID, vol_header, &block_type) != 0) {
            return 1;
        }

        switch (block_type) {
            case SIFS_UNUSED:
                SIFS_errno = SIFS_ENOTVOL; // If unused block is reference vol is malformed
                return 1;
                
            case SIFS_DIR:
                if (SIFS_readdirblock(volume, parent.entries[i].blockID, vol_header, &dir) != 0)
                {
                    return 1;
                }
                
                if (strcmp(dir.name, target_name) == 0)
                {
                    // Dir already exists
                    SIFS_errno = SIFS_EEXIST;
                    return 1;
                }

                break;
                
            case SIFS_FILE:
                if (SIFS_readfileblock(volume, parent.entries[i].blockID, vol_header, &file) != 0)
                {
                    return 1;
                }
                
                if (strcmp(file.filenames[parent.entries[i].fileindex], target_name) == 0)
                {
                    // Dir already exists
                    SIFS_errno = SIFS_EEXIST;
                    return 1;
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

    return 0;
}