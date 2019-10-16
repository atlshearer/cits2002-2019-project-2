#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

// add a copy of a new file to an existing volume
int SIFS_writefile(const char *volumename, const char *pathname,
		   void *data, size_t nbytes)
{
/*
#define	SIFS_EINVAL	1	// Invalid argument
#define	SIFS_ENOVOL	3	// No such volume
#define	SIFS_ENOENT	4	// No such file or directory entry
#define	SIFS_EEXIST	5	// Volume, file or directory already exists
#define	SIFS_EMAXENTRY	9	// Too many directory or file entries
#define	SIFS_ENOSPC	10	// No space left on volume
#define	SIFS_ENOMEM	11	// Memory allocation failed
#define	SIFS_ENOTYET	12	// Not yet implemented
*/
    FILE *vol = fopen(volumename, "r+");
    SIFS_VOLUME_HEADER header;
    char** parsed_path = NULL;
    size_t path_depth  = 0;
    unsigned char data_md5[MD5_BYTELEN];

    SIFS_DIRBLOCK curr_dir;
    SIFS_DIRBLOCK next_dir;
    SIFS_BLOCKID curr_block_id;
    SIFS_BLOCKID next_block_id;
    SIFS_BIT out_buffer = SIFS_UNUSED;

    SIFS_FILEBLOCK new_file_block;
    SIFS_BLOCKID new_block_id;
    SIFS_BLOCKID new_data_id;
    int space_found = 0;
    int block_found = 0;

    int filename_index = 0;
    int file_exists = 0;


    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }


    fread(&header, sizeof header, 1, vol);

    parsed_path = SIFS_parsepathname(pathname, &path_depth);

    MD5_buffer(data, nbytes, data_md5);

    // Read root block
    fseek(vol, header.nblocks, SEEK_CUR);
    fread(&curr_dir, sizeof curr_dir, 1, vol);
    curr_block_id = 0;

    // traverse down structure to target node
    for (size_t i = 0; i < path_depth; i++)
    {
        // check if curr dir contains next dir
        int found = 0;

        for (size_t j = 0; j < curr_dir.nentries; j++)
        {
            SIFS_BIT block_type;
            next_block_id = curr_dir.entries[j].blockID;
            fseek(vol, (sizeof(header) + next_block_id), SEEK_SET);    ////// ISSUE IN OTHER FILES
            fread(&block_type, sizeof(block_type), 1, vol);

            fseek(vol, sizeof(header) + (header.nblocks) + (curr_dir.entries[j].blockID*header.blocksize), SEEK_SET );

            printf("path_depth = %li block_type = %c %s\n", path_depth, block_type, parsed_path[i]);

            if (i != path_depth - 1 && block_type != SIFS_DIR)
            {
                SIFS_errno = SIFS_ENOTDIR;
                return 1;
            } else if (i == path_depth - 1 && block_type == SIFS_FILE) {
                fread(&new_file_block, sizeof(new_file_block), 1, vol);

                printf("%s %s\n", new_file_block.filenames[curr_dir.entries[j].fileindex], parsed_path[i]);

                if (strcmp(new_file_block.filenames[curr_dir.entries[j].fileindex], parsed_path[i]) == 0) {
                    SIFS_errno = SIFS_EEXIST;
                    return 1;
                }
            } else if (i == path_depth - 1 && block_type == SIFS_DIR) {
                fread(&next_dir, sizeof(next_dir), 1, vol);

                printf("%s %s\n", next_dir.name, parsed_path[i]);

                if (strcmp(next_dir.name, parsed_path[i]) == 0) {
                    SIFS_errno = SIFS_EEXIST;
                    return 1;
                }
            } else {
                fread(&next_dir, sizeof(next_dir), 1, vol);

                if (strcmp(next_dir.name, parsed_path[i]) == 0)
                {              
                    found = 1;
                    break;
                }
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

    // curr_dir is dir to contain new data

    if (curr_dir.nentries == SIFS_MAX_ENTRIES)
    {
        SIFS_errno = SIFS_EMAXENTRY;
        return 1;
    }


    // check if copy of file already saved
    for (size_t i = 0; i < header.nblocks; i++)
    {
        SIFS_BIT block_type;
        fseek(vol, (sizeof(header) + i), SEEK_SET);
        fread(&block_type, sizeof(block_type), 1, vol);

        if (block_type == SIFS_FILE)
        {
            fseek(vol, sizeof(header) + header.nblocks + i*header.blocksize, SEEK_SET);
            fread(&new_file_block, sizeof(new_file_block), 1, vol);

            if (strcmp(MD5_format(new_file_block.md5), MD5_format(data_md5)) == 0)
            {
                printf("File match %s %s\n", MD5_format(new_file_block.md5), MD5_format(data_md5));

                // Same file
                if (new_file_block.nfiles >= SIFS_MAX_ENTRIES)
                {
                    SIFS_errno = SIFS_EMAXENTRY;
                    return 1;
                }

                file_exists = 1;
                new_block_id = i;
                
                strcpy(new_file_block.filenames[new_file_block.nfiles], parsed_path[path_depth - 1]);
                filename_index = new_file_block.nfiles;
                new_file_block.nfiles++;

                fseek(vol, sizeof(header) + header.nblocks + i*header.blocksize, SEEK_SET);
                fwrite(&new_file_block, sizeof(new_file_block), 1, vol);

                break;
            }
        }
    }

    if (!file_exists)
    {  
    
        // find space for obj
        fseek(vol, sizeof(header), SEEK_SET);
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

        for (size_t i = new_block_id + 1; i < header.nblocks; i++)
        {
            SIFS_BIT block_type;
            fread(&block_type, sizeof(block_type), 1, vol);

            printf("i = %li  block_type = %c  space_found = %i\n", i, block_type, space_found);

            if (block_type == SIFS_UNUSED)
            {
                if (space_found == 0)
                {
                    new_data_id = i;
                }
                
                space_found++;

                if (space_found * header.blocksize >= nbytes)
                {
                    break;
                }
            } else {
                space_found = 0;
            }
        }



        if (!space_found || !block_found)
        {
            SIFS_errno = SIFS_ENOSPC;
            return 1;
        }

        printf("new_block_id = %i  new_data_id = %i space_found = %i\n", new_block_id, new_data_id, space_found);

        // create file block
        memset(&new_file_block, 0, sizeof(new_file_block));
        new_file_block.modtime = time(NULL);
        new_file_block.length = nbytes;
        memcpy(new_file_block.md5, data_md5, MD5_BYTELEN);
        new_file_block.firstblockID = new_data_id;
        new_file_block.nfiles = 1;
        strcpy(new_file_block.filenames[0], parsed_path[path_depth - 1]);
        
        // write file block
        fseek(vol, sizeof(header) + header.nblocks + new_block_id*header.blocksize, SEEK_SET);
        fwrite(&new_file_block, sizeof(new_file_block), 1, vol);

        // write data
        fseek(vol, sizeof(header) + header.nblocks + new_data_id*header.blocksize, SEEK_SET);
        fwrite(data, nbytes, 1, vol);
        

        // update bitmap
        out_buffer = SIFS_FILE;
        fseek(vol, sizeof(header) + new_block_id, SEEK_SET);
        fwrite(&out_buffer, sizeof(out_buffer), 1, vol);

        out_buffer = SIFS_DATABLOCK;
        fseek(vol, sizeof(header) + new_data_id, SEEK_SET);
        for (size_t i = 0; i < space_found; i++)
        {
            fwrite(&out_buffer, sizeof(out_buffer), 1, vol);
        }

    }
    
    // update parent dir
    curr_dir.entries[curr_dir.nentries].blockID = new_block_id;
    curr_dir.entries[curr_dir.nentries].fileindex = filename_index;
    curr_dir.nentries++;

    // write parent dir
    fseek(vol, sizeof(header) + header.nblocks + curr_block_id*header.blocksize, SEEK_SET);
    fwrite(&curr_dir, sizeof(curr_dir), 1, vol);




    fclose(vol);

    // Free parsed_path
    for (size_t i = 0; *(parsed_path + i); i++)
    {
        free(*(parsed_path + i));
    }
    free(parsed_path);

    SIFS_errno	= SIFS_ENOTYET;
    return 1;
}
