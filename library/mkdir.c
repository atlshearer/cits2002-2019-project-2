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

    SIFS_DIRBLOCK curr_dir;
    SIFS_DIRBLOCK next_dir;
    SIFS_DIRBLOCK new_dir;
    SIFS_BLOCKID curr_block_id;
    SIFS_BLOCKID next_block_id;
    SIFS_BLOCKID new_block_id;
    int block_found = 0;
    

    //  VOLUME CREATION FAILED
    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }

    

    fread(&header, sizeof header, 1, vol);

    printf("blocksize = %4li  nblocks = %6i\n", header.blocksize, header.nblocks);

    // Split pathname on /
    parsed_path = SIFS_parsepathname(pathname, &path_depth);

    for (size_t i = 0; *(parsed_path + i); i++)
    {
        printf("%s\n", *(parsed_path + i));
    }
    

    printf("path_depth = %li\n", path_depth);

    // read root block
    fseek(vol, header.nblocks, SEEK_CUR);
    fread(&curr_dir, sizeof curr_dir, 1, vol);
    curr_block_id = 0;

    printf("name = %s  modtime = %li  nentires = %i\n", curr_dir.name, curr_dir.modtime, curr_dir.nentries);


    // traverse down dir structure to leaf node
    for (size_t i = 0; i < path_depth; i++)
    {
        // check if curr dir contains next dir
        int found = 0;

        for (size_t j = 0; j < curr_dir.nentries; j++)
        {
            // NOT CHECKING BLOCK TYPE ######################################
            fseek(vol, (sizeof header) + (header.nblocks) + (curr_dir.entries[j].blockID*header.blocksize), SEEK_SET );
            fread(&next_dir, sizeof next_dir, 1, vol);
            next_block_id = curr_dir.entries[j].blockID;

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
    fseek(vol, sizeof header, SEEK_SET);
    for (size_t i = 0; i < header.nblocks; i++)
    {
        SIFS_BIT block_type;
        fread(&block_type, sizeof(block_type), 1, vol);

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

    printf("Empty block found at BLOCKID = %i\n", new_block_id);

    // write to vol
    fseek(vol, sizeof(header) + header.nblocks + new_block_id*header.blocksize, SEEK_SET);
    int nwritten = fwrite(&new_dir, sizeof(new_dir), 1, vol);
    printf("nwritten = %i\n", nwritten);

    // update parent dir
    curr_dir.entries[curr_dir.nentries].blockID = new_block_id;
    curr_dir.nentries++;
    
    fseek(vol, (sizeof header) + (header.nblocks) + curr_block_id*header.blocksize, SEEK_SET);
    fwrite(&curr_dir, sizeof curr_dir, 1, vol);
    
    // update bitmap
    char bitmap_val = SIFS_DIR;
    fseek(vol, sizeof(header) + new_block_id, SEEK_SET);
    fwrite(&bitmap_val, sizeof bitmap_val, 1, vol);


    fclose(vol);

    // Free parsed_path
    for (size_t i = 0; *(parsed_path + i); i++)
    {
        free(*(parsed_path + i));
    }
    free(parsed_path);
    
    return 0;
}

