//  CITS2002 Project 2 2019
//  Name(s):             Alexander Shearer, Thomas Kinsella
//  Student number(s):   22465777, 22177293

#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

// make a new directory within an existing volume
int SIFS_mkdir(const char *volumename, const char *pathname)
{  
    FILE *vol = fopen(volumename, "r+");
    SIFS_VOLUME_HEADER header;
    char** parsed_path = NULL;
    size_t path_depth  = 0;

    SIFS_DIRBLOCK parent_dir;
    SIFS_DIRBLOCK new_dir;
    SIFS_BLOCKID parent_dir_id;
    SIFS_BLOCKID new_block_id;
    int block_found = 0;
    
    SIFS_BIT block_type;
    

    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }

    if (SIFS_readheader(vol, &header) != 0) {
        SIFS_errno = SIFS_ENOTVOL;
        return 1;
    }

    if (SIFS_checkvolumeintegrity(vol, header) != 0) 
    {
        return 1;
    }

    if (SIFS_parsepathname(pathname, &parsed_path, &path_depth) != 0) 
    {
        return 1;
    }

    if (path_depth < 1) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // Retrieve parent block
    if (SIFS_getdirblockid(vol, parsed_path, path_depth - 1, header, &parent_dir_id) != 0)
    {
        return 1;
    }

    if (SIFS_readdirblock(vol, parent_dir_id, header, &parent_dir) != 0)
    {
        return 1;
    }
    
    // Check if file or dir already exists
    if (SIFS_checkexists(vol, parent_dir, parsed_path[path_depth - 1], header) != 0)
    {
        return 1;
    }   

    //
    if (parent_dir.nentries == SIFS_MAX_ENTRIES)
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
        if (SIFS_readblocktype(vol, i, header, &block_type) != 0)
        {
            return 1;
        }

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
    if (SIFS_writedirblock(vol, new_block_id, header, &new_dir) != 0)
    {
        return 1;
    }

    // update parent dir
    parent_dir.entries[parent_dir.nentries].blockID = new_block_id;
    parent_dir.modtime = time(NULL);
    parent_dir.nentries++;
    
    if (SIFS_writedirblock(vol, parent_dir_id, header, &parent_dir) != 0)
    {
        return 1;
    }
    
    // update bitmap
    block_type = SIFS_DIR;
    if (SIFS_writeblocktype(vol, new_block_id, header, &block_type, 1) != 0)
    {
        return 1;
    }
    

    // cleanup
    fclose(vol);

    SIFS_freeparsedpath(parsed_path);
    parsed_path = NULL;
    
    return 0;
}

