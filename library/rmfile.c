#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

// remove an existing file from an existing volume
int SIFS_rmfile(const char *volumename, const char *pathname)
{
    /*
     #define    SIFS_EINVAL    1    // Invalid argument
     #define    SIFS_ENOVOL    3    // No such volume
     #define    SIFS_ENOENT    4    // No such file or directory entry
     #define    SIFS_ENOTVOL    6    // Not a volume
     #define    SIFS_ENOTDIR    7    // Not a directory
     #define    SIFS_ENOTFILE    8    // Not a file
     #define    SIFS_ENOMEM    11    // Memory allocation failed
     */

    FILE *vol = fopen(volumename, "r+");
    SIFS_VOLUME_HEADER header;
    char **parsed_path = NULL;
    size_t path_depth = 0;

    SIFS_DIRBLOCK dir_buffer;
    SIFS_DIRBLOCK parent;
    SIFS_FILEBLOCK target;

    SIFS_BLOCKID parent_id;
    SIFS_BLOCKID target_id;

    SIFS_BIT bit_buffer;

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

    if (path_depth < 1) {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    if (SIFS_getfileblockid(vol, parsed_path, path_depth, header, &parent_id, &target_id) != 0)
    {
        return 1;
    }

    if (SIFS_readdirblock(vol, parent_id, header, &parent) != 0)
    {
        return 1;
    }

    if (SIFS_readfileblock(vol, target_id, header, &target) != 0)
    {
        return 1;
    }

    // find target index
    for (size_t i = 0; i < parent.nentries; i++)
    {
        if (parent.entries[i].blockID == target_id &&
            strcmp(target.filenames[parent.entries[i].fileindex], parsed_path[path_depth - 1]) == 0)
        {
            // Check if other copies exist
            if (target.nfiles > 1)
            {
                // Remove filename from target
                for (size_t j = parent.entries[i].fileindex; j < target.nfiles - 1; j++)
                {
                    strcpy(target.filenames[j], target.filenames[j + 1]);
                }

                target.nfiles--;

                // Update fileindex in all dir that 'contain' target
                for (size_t j = 0; j < header.blocksize; j++)
                {
                    if (SIFS_readblocktype(vol, j, header, &bit_buffer) != 0)
                    {
                        return 1;
                    }

                    if (bit_buffer == SIFS_DIR)
                    {
                        if (SIFS_readdirblock(vol, j, header, &dir_buffer) != 0)
                        {
                            return 1;
                        }

                        for (size_t entry_idx = 0; entry_idx < dir_buffer.nentries; entry_idx++)
                        {
                            if (dir_buffer.entries[entry_idx].blockID == target_id && 
                                dir_buffer.entries[entry_idx].fileindex > parent.entries[i].fileindex)
                            {
                                dir_buffer.entries[entry_idx].fileindex--;
                            }
                        }

                        if (SIFS_writedirblock(vol, j, header, &dir_buffer) != 0)
                        {
                            return 1;
                        }
                    }
                }
                
                // Write back to file
                if (SIFS_writefileblock(vol, target_id, header, &target) != 0)
                {
                    return 1;
                }
            }
            else
            {
                // Remove file from archive
                bit_buffer = SIFS_UNUSED;
                if (SIFS_writeblocktype(vol, target_id, header, &bit_buffer, 1) != 0)
                {
                    return 1;
                }

                if (SIFS_writeblocktype(vol, target.firstblockID, header, &bit_buffer, (target.length + header.blocksize - 1) / header.blocksize) != 0)
                {
                    return 1;
                }
            }

            // Is possible that fileindex will have been altered, need to refresh parent
            if (SIFS_readdirblock(vol, parent_id, header, &parent) != 0)
            {
                return 1;
            }

            // Remove reference in parent
            for (size_t j = i; j < parent.nentries - 1; j++)
            {
                parent.entries[j] = parent.entries[j + 1];
            }

            parent.nentries--;
            parent.modtime = time(NULL);

            // Write back to file
            if (SIFS_writedirblock(vol, parent_id, header, &parent) != 0)
            {
                return 1;
            }

            break;
        }
    }

    fclose(vol);

    SIFS_errno = SIFS_EOK;
    return 0;
}
