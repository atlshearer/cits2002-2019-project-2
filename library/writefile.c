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
    
    unsigned char data_md5[MD5_BYTELEN];
    char* old_md5 = malloc(sizeof(char) * (MD5_STRLEN + 1));
    char* new_md5 = malloc(sizeof(char) * (MD5_STRLEN + 1));

    if (!old_md5 || !new_md5) {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }

    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }

    SIFS_getheader(vol, &header);

    parsed_path = SIFS_parsepathname(pathname, &path_depth);

    MD5_buffer(data, nbytes, data_md5);
    memcpy(new_md5, MD5_format(data_md5), MD5_STRLEN);
    new_md5[MD5_STRLEN] = 0;

    // Read root block
    if(SIFS_getdirblock(vol, SIFS_ROOTDIR_BLOCKID, header, &curr_dir) != 0) {
        return 1;
    }
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
            ////// ISSUE IN OTHER FILES #####################
            SIFS_getblocktype(vol, next_block_id, header, &block_type);

            if (i != path_depth - 1 && block_type != SIFS_DIR)
            {
                SIFS_errno = SIFS_ENOTDIR;
                return 1;
            } else if (i == path_depth - 1 && block_type == SIFS_FILE) {
                SIFS_getfileblock(vol, curr_dir.entries[j].blockID, header, &new_file_block);

                if (strcmp(new_file_block.filenames[curr_dir.entries[j].fileindex], parsed_path[i]) == 0) {
                    SIFS_errno = SIFS_EEXIST;
                    return 1;
                }
            } else if (i == path_depth - 1 && block_type == SIFS_DIR) {
                if (SIFS_getdirblock(vol, curr_dir.entries[j].blockID, header, &next_dir) != 0) {
                    return 1;
                }

                if (strcmp(next_dir.name, parsed_path[i]) == 0) {
                    SIFS_errno = SIFS_EEXIST;
                    return 1;
                }
            } else {
                if(SIFS_getdirblock(vol, curr_dir.entries[j].blockID, header, &next_dir) != 0) {
                    return 1;
                }

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
        SIFS_getblocktype(vol, i, header, &block_type);

        if (block_type == SIFS_FILE)
        {
            if (SIFS_getfileblock(vol, i, header, &new_file_block) != 0) {
                return 1;
            };
            
            memcpy(old_md5, MD5_format(new_file_block.md5), MD5_STRLEN);
            old_md5[MD5_STRLEN] = 0;
            
            if (strcmp(old_md5, new_md5) == 0)
            {
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

                SIFS_writefileblock(vol, i, header, &new_file_block);

                break;
            }
        }
    }

    if (!file_exists)
    {  
    
        // find space for obj
        for (size_t i = 0; i < header.nblocks; i++)
        {
            SIFS_BIT block_type;
            SIFS_getblocktype(vol, i, header, &block_type);
            
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
        
        for (size_t i = 0; i < header.nblocks; i++)
        {
            if (new_data_id <= i && i < new_data_id + space_found) {
                // overlap with space found for data
                continue;
            }
            
            SIFS_BIT block_type;
            SIFS_getblocktype(vol, i, header, &block_type);

            if (block_type == SIFS_UNUSED)
            {
                block_found = 1;
                new_block_id = i;
                break;
            }
        }

        if (!space_found || !block_found)
        {
            SIFS_errno = SIFS_ENOSPC;
            return 1;
        }

        // create file block
        memset(&new_file_block, 0, sizeof(new_file_block));
        new_file_block.modtime = time(NULL);
        new_file_block.length = nbytes;
        memcpy(new_file_block.md5, data_md5, MD5_BYTELEN);
        new_file_block.firstblockID = new_data_id;
        new_file_block.nfiles = 1;
        strcpy(new_file_block.filenames[0], parsed_path[path_depth - 1]);
        
        // write file block
        SIFS_writefileblock(vol, new_block_id, header, &new_file_block);

        // write data
        SIFS_writedata(vol, new_data_id, header, data, nbytes);

        // update bitmap
        out_buffer = SIFS_FILE;
        SIFS_writeblocktype(vol, new_block_id, header, &out_buffer, 1);

        out_buffer = SIFS_DATABLOCK;
        SIFS_writeblocktype(vol, new_data_id, header, &out_buffer, space_found);
    }
    
    // update parent dir
    curr_dir.entries[curr_dir.nentries].blockID = new_block_id;
    curr_dir.entries[curr_dir.nentries].fileindex = filename_index;
    curr_dir.nentries++;

    // write parent dir
    SIFS_writedirblock(vol, curr_block_id, header, &curr_dir);


    // cleanup
    fclose(vol);

    // Free parsed_path
    for (size_t i = 0; *(parsed_path + i); i++)
    {
        free(*(parsed_path + i));
    }
    free(parsed_path);

    // Free MD5 str stores
    free(old_md5);
    free(new_md5);
    
    return 0;
}

