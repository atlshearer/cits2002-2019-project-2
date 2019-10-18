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

    SIFS_DIRBLOCK target_dir;
    SIFS_DIRBLOCK dir_buffer;

    SIFS_BLOCKID target_dir_id;

    SIFS_BIT block_type;
    SIFS_FILEBLOCK file_block;
    
    char* name_cpy;


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

    if (SIFS_getdirblockid(vol, parsed_path, path_depth, header, &target_dir_id) != 0)
    {
        return 1;
    }

    if (SIFS_readdirblock(vol, target_dir_id, header, &target_dir) != 0)
    {
        return 1;
    }
    
    
    *nentries = target_dir.nentries;
    *modtime = target_dir.modtime;
    *entrynames = malloc(sizeof(char *) * (target_dir.nentries + 1));

    if (*entrynames == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }


    for (size_t i = 0; i < target_dir.nentries; i++)
    {        
        if (SIFS_readblocktype(vol, target_dir.entries[i].blockID, header, &block_type) != 0) {
            return 1;
        }
        
        switch (block_type) {
            case SIFS_UNUSED:
                SIFS_errno = SIFS_ENOTVOL; // If unused block is reference vol is malformed
                return 1;
                
            case SIFS_DIR:
                if (SIFS_readdirblock(vol, target_dir.entries[i].blockID, header, &dir_buffer) != 0)
                {
                    return 1;
                }

                name_cpy = malloc(strlen(dir_buffer.name));

                if (name_cpy == NULL)
                {
                    SIFS_errno = SIFS_ENOMEM;
                    return 1;
                }
                strcpy(name_cpy, dir_buffer.name);
                
                break;
                
            case SIFS_FILE:
                if (SIFS_readfileblock(vol, target_dir.entries[i].blockID, header, &file_block) != 0)
                {
                    return 1;
                }
                
                name_cpy = malloc(strlen(file_block.filenames[target_dir.entries[i].fileindex]));

                if (name_cpy == NULL)
                {
                    SIFS_errno = SIFS_ENOMEM;
                    return 1;
                }
                strcpy(name_cpy, file_block.filenames[target_dir.entries[i].fileindex]);
                
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
    SIFS_freeparsedpath(parsed_path);
    parsed_path = NULL;

    SIFS_errno	= SIFS_ENOTYET;
    return 0;
}

