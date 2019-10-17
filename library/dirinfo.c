#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

// get information about a requested directory
int SIFS_dirinfo(const char *volumename, const char *pathname,
                 char ***entrynames, uint32_t *nentries, time_t *modtime)
{
    /*
#define    SIFS_EINVAL    1    // Invalid argument
#define    SIFS_ENOVOL    3    // No such volume
#define    SIFS_ENOENT    4    // No such file or directory entry
#define    SIFS_ENOTVOL    6    // Not a volume
#define    SIFS_ENOTDIR    7    // Not a directory
#define    SIFS_ENOMEM    11    // Memory allocation failed
     */
    
    FILE *vol = fopen(volumename, "r");
    SIFS_VOLUME_HEADER header;
    char** parsed_path = NULL;
    size_t path_depth  = 0;

    SIFS_DIRBLOCK curr_dir;
    SIFS_DIRBLOCK next_dir;


    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }

    SIFS_getheader(vol, &header);

    parsed_path = SIFS_parsepathname(pathname, &path_depth);

    // Read root block
    if (SIFS_getdirblock(vol, SIFS_ROOTDIR_BLOCKID, header, &curr_dir)) {
        return 1;
    }

    // traverse down structure to target node
    for (size_t i = 0; i < path_depth; i++)
    {
        // check if curr dir contains next dir
        int found = 0;

        for (size_t j = 0; j < curr_dir.nentries; j++)
        {
            SIFS_BIT block_type;
            if (SIFS_getblocktype(vol, curr_dir.entries[j].blockID, header, &block_type) != 0) {
                return 1;
            }
            
            switch (block_type) {
                case SIFS_UNUSED:
                    SIFS_errno = SIFS_ENOTVOL; // If unused block is reference vol is malformed
                    return 1;
                
                case SIFS_DIR:
                    SIFS_getdirblock(vol, curr_dir.entries[j].blockID, header, &next_dir);
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
                found = 1;
                break;
            }
        }

        if (found) { // move into dir
            curr_dir = next_dir;
        } else {
            SIFS_errno = SIFS_ENOENT;
            return 1;
        }
    }


    // in target dir
    *nentries = curr_dir.nentries;
    *modtime = curr_dir.modtime;
    *entrynames = malloc(sizeof(char *) * (curr_dir.nentries + 1));
    if (*entrynames == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }


    for (size_t i = 0; i < curr_dir.nentries; i++)
    {
        SIFS_BIT block_type;
        char* name_cpy;
        SIFS_FILEBLOCK file_block;
        
        if (SIFS_getblocktype(vol, curr_dir.entries[i].blockID, header, &block_type) != 0) {
            return 1;
        }
        
        switch (block_type) {
            case SIFS_UNUSED:
                SIFS_errno = SIFS_ENOTVOL; // If unused block is reference vol is malformed
                return 1;
                
            case SIFS_DIR:
                SIFS_getdirblock(vol, curr_dir.entries[i].blockID, header, &next_dir);
                
                name_cpy = malloc(strlen(next_dir.name));
                if (name_cpy == NULL)
                {
                    SIFS_errno = SIFS_ENOMEM;
                    return 1;
                }
                strcpy(name_cpy, next_dir.name);
                
                break;
                
            case SIFS_FILE:
                SIFS_getfileblock(vol, curr_dir.entries[i].blockID, header, &file_block);
                
                name_cpy = malloc(strlen(file_block.filenames[curr_dir.entries[i].fileindex]));
                if (name_cpy == NULL)
                {
                    SIFS_errno = SIFS_ENOMEM;
                    return 1;
                }
                strcpy(name_cpy, file_block.filenames[curr_dir.entries[i].fileindex]);
                
                break;
                
            case SIFS_DATABLOCK:
                SIFS_errno = SIFS_ENOTVOL; // If datablock is reference vol is malformed
                return 1;
                
            default:
                SIFS_errno = SIFS_ENOTVOL; // If blocktype is invalid vol is malformed
                return 1;
        }
        
        
        *(*entrynames + i) = name_cpy;
    }

    fclose(vol);
    
    // Free parsed_path
    for (size_t i = 0; *(parsed_path + i); i++)
    {
        free(*(parsed_path + i));
    }
    free(parsed_path);

    SIFS_errno	= SIFS_ENOTYET;
    return 0;
}

