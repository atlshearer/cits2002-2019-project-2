#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

// make a new directory within an existing volume
int SIFS_mkdir(const char *volumename, const char *pathname)
{
    /*
#define    SIFS_EINVAL    1    // Invalid argument
#define    SIFS_ENOVOL    3    // No such volume
#define    SIFS_ENOENT    4    // No such file or directory entry
#define    SIFS_EEXIST    5    // Volume, file or directory already exists
#define    SIFS_ENOTVOL    6    // Not a volume
#define    SIFS_ENOTDIR    7    // Not a directory
#define    SIFS_EMAXENTRY    9    // Too many directory or file entries
#define    SIFS_ENOSPC    10    // No space left on volume
#define    SIFS_ENOMEM    11    // Memory allocation failed
     */
    
    FILE *vol = fopen(volumename, "r+");
    SIFS_VOLUME_HEADER header;
    char** parsed_path = NULL;
    size_t path_depth  = 0;

    SIFS_DIRBLOCK curr_dir;
    SIFS_DIRBLOCK next_dir;
    SIFS_DIRBLOCK new_dir;
    SIFS_BLOCKID curr_block_id;
    SIFS_BLOCKID next_block_id;
    SIFS_BLOCKID new_block_id;
    int block_found = 0;
    
    SIFS_BIT bitmap_buffer;
    

    //  VOLUME CREATION FAILED
    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }

    if (SIFS_getheader(vol, &header) != 0) {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }

    parsed_path = SIFS_parsepathname(pathname, &path_depth);

    // read root block
    SIFS_getdirblock(vol, SIFS_ROOTDIR_BLOCKID, header, &curr_dir);
    curr_block_id = 0;

    // traverse down dir structure to leaf node
    for (size_t i = 0; i < path_depth; i++)
    {
        // check if curr dir contains next dir
        int found = 0;

        for (size_t j = 0; j < curr_dir.nentries; j++)
        {
            next_block_id = curr_dir.entries[j].blockID;
            
            SIFS_BIT block_type;
            if (SIFS_getblocktype(vol, next_block_id, header, &block_type) != 0) {
                return 1;
            }
            
            switch (block_type) {
                case SIFS_UNUSED:
                    SIFS_errno = SIFS_ENOTVOL; // If unused block is reference vol is malformed
                    return 1;
                    
                case SIFS_DIR:
                    SIFS_getdirblock(vol, next_block_id, header, &next_dir);
                    break;
                    
                case SIFS_FILE:
                    SIFS_errno = SIFS_ENOTDIR;
                    return 1;
                    
                case SIFS_DATABLOCK:
                    SIFS_errno = SIFS_ENOTVOL; // If datablock is reference vol is malformed
                    return 1;
                    
                default:
                    SIFS_errno = SIFS_ENOTVOL; // If blocktype is invalid vol is malformed
                    return 1;
            }
            

            if (strcmp(next_dir.name, parsed_path[i]) == 0)
            {
                if (i == path_depth - 1)
                {
                    // have reached target parent, however new dir already exists
                    SIFS_errno = SIFS_EEXIST;
                    return 1;
                }
                
                found = 1;
                break;
            }
        }

        
        if (found) { // move into dir
            curr_dir = next_dir;
            curr_block_id = next_block_id;
        } else if (i == path_depth - 1) {
            // nothing found, in parent dir, target dir not existant
        } else {
            SIFS_errno = SIFS_ENOENT;
            return 1;
        }
    }

    // will be in dir that new dir should be made in
    if (curr_dir.nentries == SIFS_MAX_ENTRIES)
    {
        SIFS_errno = SIFS_EMAXENTRY;
        return 1;
    }

    // create dir object to be written    
    memset(&new_dir, 0, sizeof(new_dir));
    strcpy(new_dir.name, parsed_path[path_depth - 1]);
    new_dir.modtime = time(NULL);
    new_dir.nentries = 0;

    // find space for obj
    for (size_t i = 0; i < header.nblocks; i++)
    {
        SIFS_BIT block_type;
        SIFS_getblocktype(vol, i, header, &block_type);

        if (block_type == SIFS_UNUSED)
        {
            block_found = 1;
            new_block_id = i;
            break;
        }
    }

    if (!block_found)
    {
        SIFS_errno = SIFS_ENOSPC;
        return 1;
    }

    // write to vol
    SIFS_writedirblock(vol, new_block_id, header, &new_dir);

    // update parent dir
    curr_dir.entries[curr_dir.nentries].blockID = new_block_id;
    curr_dir.nentries++;
    
    SIFS_writedirblock(vol, curr_block_id, header, &curr_dir);
    
    // update bitmap
    bitmap_buffer = SIFS_DIR;
    SIFS_writeblocktype(vol, new_block_id, header, &bitmap_buffer, 1);


    fclose(vol);

    // Free parsed_path
    for (size_t i = 0; *(parsed_path + i); i++)
    {
        free(*(parsed_path + i));
    }
    free(parsed_path);
    
    return 0;
}

