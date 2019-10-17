#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

// remove an existing directory from an existing volume
int SIFS_rmdir(const char *volumename, const char *pathname)
{
    /*
#define	SIFS_EINVAL	1	// Invalid argument  (trying to rm / )
#define	SIFS_ENOVOL	3	// No such volume
#define	SIFS_ENOENT	4	// No such file or directory entry
#define	SIFS_ENOTDIR	7	// Not a directory
#define	SIFS_ENOMEM	11	// Memory allocation failed
#define	SIFS_ENOTEMPTY	13	// Directory is not empty
    */
    FILE *vol = fopen(volumename, "r+");
    SIFS_VOLUME_HEADER header;
    char** parsed_path = NULL;
    size_t path_depth  = 0;

    SIFS_DIRBLOCK curr_dir;
    SIFS_DIRBLOCK next_dir;
    SIFS_BLOCKID curr_block_id;
    SIFS_BLOCKID next_block_id;
    SIFS_BIT out_buffer;

    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }

    SIFS_getheader(vol, &header);

    parsed_path = SIFS_parsepathname(pathname, &path_depth);
    
    if (path_depth == 0) {
        SIFS_errno = SIFS_EINVAL; // trying to rm /
        return 1;
    }

    // Read root block
    SIFS_getdirblock(vol, SIFS_ROOTDIR_BLOCKID, header, &curr_dir);
    curr_block_id = 0;

    // trverse down structure to target node
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
                    SIFS_errno = SIFS_ENOTDIR; // Could be part of path or target 'dir'
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
                found = 1;
                break;
            }
        }

        
        if (found && i != path_depth - 1) { // move into dir
            curr_dir = next_dir;
            curr_block_id = next_block_id;
        } else if (found) { // do nothing, target dir in next_dir
            
        } else {
            SIFS_errno = SIFS_ENOENT;
            return 1;
        }
    }

    if (next_dir.nentries != 0)
    {
        SIFS_errno = SIFS_ENOTEMPTY;
        return 1;
    }

    // Clear bitmap
    out_buffer = SIFS_UNUSED;
    SIFS_writeblocktype(vol, next_block_id, header, &out_buffer, 1);

    // Clear reference in parent
    for (size_t i = 0; i < curr_dir.nentries; i++)
    {
        if (curr_dir.entries[i].blockID == next_block_id)
        {
            for (size_t j = i; j < curr_dir.nentries - 1; j++)
            {
                curr_dir.entries[j] = curr_dir.entries[j + 1];
            }
            break;
        }
        
    }
    curr_dir.nentries--;
    
    // Write parent
    SIFS_writedirblock(vol, curr_block_id, header, &curr_dir);

    // cleanup
    
    fclose(vol);

    SIFS_errno	= SIFS_EOK;
    return 0;
}

