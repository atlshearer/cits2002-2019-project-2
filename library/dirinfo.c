#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

// get information about a requested directory
int SIFS_dirinfo(const char *volumename, const char *pathname,
                 char ***entrynames, uint32_t *nentries, time_t *modtime)
{
    
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

    fread(&header, sizeof header, 1, vol);

    parsed_path = SIFS_parsepathname(pathname, &path_depth);


    // Read root block
    fseek(vol, header.nblocks, SEEK_CUR);
    fread(&curr_dir, sizeof curr_dir, 1, vol);

    // trverse down structure to target node
    for (size_t i = 0; i < path_depth; i++)
    {
        // check if curr dir contains next dir
        int found = 0;

        for (size_t j = 0; j < curr_dir.nentries; j++)
        {
            fseek(vol, (sizeof header) + (header.nblocks) + (curr_dir.entries[j].blockID*header.blocksize), SEEK_SET );
            fread(&next_dir, sizeof next_dir, 1, vol);

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
        // NOT CHECKING IF IS DIR ################################################
        fseek(vol, (sizeof header) + (header.nblocks) + (curr_dir.entries[i].blockID*header.blocksize), SEEK_SET );
        fread(&next_dir, sizeof next_dir, 1, vol); // Reuse next_dir variable

        char* name_cpy = malloc(strlen(next_dir.name));
        if (name_cpy == NULL)
        {
            SIFS_errno = SIFS_ENOMEM;
            return 1;
        }
        strcpy(name_cpy, next_dir.name);
        *(*entrynames + i) = name_cpy;
    }

    fclose(vol);

    SIFS_errno	= SIFS_ENOTYET;
    return 0;
}
