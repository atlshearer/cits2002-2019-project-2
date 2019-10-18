//  CITS2002 Project 2 2019
//  Name(s):             Alexander Shearer, Thomas Kinsella
//  Student number(s):   22465777, 22177293

#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

// remove an existing directory from an existing volume
int SIFS_rmdir(const char *volumename, const char *pathname)
{
    FILE *vol = fopen(volumename, "r+");
    SIFS_VOLUME_HEADER header;
    char** parsed_path = NULL;
    size_t path_depth  = 0;

    SIFS_DIRBLOCK parent_dir;
    SIFS_DIRBLOCK target_dir;
    SIFS_BLOCKID parent_dir_id;
    SIFS_BLOCKID target_dir_id;
    SIFS_BIT block_type;

    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }

    if (SIFS_readheader(vol, &header) != 0)
    {
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
    
    if (path_depth == 0) {
        SIFS_errno = SIFS_EINVAL; // trying to rm /
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

    // Retrieve target block
    if (SIFS_getdirblockid(vol, parsed_path, path_depth, header, &target_dir_id) != 0)
    {
        return 1;
    }

    if (SIFS_readdirblock(vol, target_dir_id, header, &target_dir) != 0)
    {
        return 1;
    }

    // Check empty
    if (target_dir.nentries != 0)
    {
        SIFS_errno = SIFS_ENOTEMPTY;
        return 1;
    }

    // Clear bitmap
    block_type = SIFS_UNUSED;
    if (SIFS_writeblocktype(vol, target_dir_id, header, &block_type, 1) != 0)
    {
        return 0;
    }

    // Clear reference in parent
    for (size_t i = 0; i < parent_dir.nentries; i++)
    {
        if (parent_dir.entries[i].blockID == target_dir_id)
        {
            for (size_t j = i; j < parent_dir.nentries - 1; j++)
            {
                parent_dir.entries[j] = parent_dir.entries[j + 1];
            }
            break;
        }
        
    }
    parent_dir.nentries--;
    parent_dir.modtime = time(NULL);
    
    // Write parent
    if (SIFS_writedirblock(vol, parent_dir_id, header, &parent_dir) != 0)
    {
        return 1;
    }

    // cleanup
    fclose(vol);
    
    SIFS_freeparsedpath(parsed_path);
    parsed_path = NULL;

    SIFS_errno	= SIFS_EOK;
    return 0;
}

