//  CITS2002 Project 2 2019
//  Name(s):             Alexander Shearer, Thomas Kinsella
//  Student number(s):   22465777, 22177293

#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

// add a copy of a new file to an existing volume
int SIFS_writefile(const char *volumename, const char *pathname,
		   void *data, size_t nbytes)
{
    FILE *vol = fopen(volumename, "r+");
    SIFS_VOLUME_HEADER header;
    char** parsed_path = NULL;
    size_t path_depth  = 0;

    SIFS_DIRBLOCK parent_dir;
    SIFS_BLOCKID parent_dir_id;
    SIFS_BIT out_buffer = SIFS_UNUSED;

    SIFS_FILEBLOCK new_file_block;
    SIFS_BLOCKID new_block_id;
    SIFS_BLOCKID new_data_id;
    int space_found = 0;
    int block_found = 0;

    int filename_index = 0;
    int file_exists = 0;

    SIFS_BIT block_type;
    
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

    MD5_buffer(data, nbytes, data_md5);
    memcpy(new_md5, MD5_format(data_md5), MD5_STRLEN);
    new_md5[MD5_STRLEN] = 0;

    // Retreive parent_dir
    if (SIFS_getdirblockid(vol, parsed_path, path_depth - 1, header, &parent_dir_id) != 0)
    {    
        return 1;
    }

    if (SIFS_readdirblock(vol, parent_dir_id, header, &parent_dir) != 0)
    {
        return 1;
    }

    // Check if file or dir exists
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

    // check if copy of file already saved
    for (size_t i = 0; i < header.nblocks; i++)
    {
        SIFS_BIT block_type;
        if (SIFS_readblocktype(vol, i, header, &block_type) != 0)
        {
            return 1;
        }

        if (block_type == SIFS_FILE)
        {
            if (SIFS_readfileblock(vol, i, header, &new_file_block) != 0) {
                return 1;
            };
            
            memcpy(old_md5, MD5_format(new_file_block.md5), MD5_STRLEN);
            old_md5[MD5_STRLEN] = 0;
            
            if (strcmp(old_md5, new_md5) == 0)
            {
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

                if (SIFS_writefileblock(vol, i, header, &new_file_block) != 0)
                {
                    return 1;
                }

                break;
            }
        }
    }

    if (!file_exists)
    {  
        // find space for obj
        for (size_t i = 0; i < header.nblocks; i++)
        {
            if (SIFS_readblocktype(vol, i, header, &block_type) != 0)
            {
                return 1;
            }
            
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
            if (new_data_id <= i && i < new_data_id + space_found && nbytes > 0) {
                // overlap with space found for data
                continue;
            }
            
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
        if (SIFS_writefileblock(vol, new_block_id, header, &new_file_block) != 0)
        {
            return 1;
        }

        // write data
        if (SIFS_writedata(vol, new_data_id, header, data, nbytes) != 0)
        {
            return 1;
        }

        // update bitmap
        out_buffer = SIFS_FILE;
        if (SIFS_writeblocktype(vol, new_block_id, header, &out_buffer, 1) != 0)
        {
            return 1;
        }

        out_buffer = SIFS_DATABLOCK;
        if (nbytes > 0)
        {
            if (SIFS_writeblocktype(vol, new_data_id, header, &out_buffer, space_found) != 0)
            {
                return 1;
            }
        }
    }
    
    // update parent dir
    parent_dir.entries[parent_dir.nentries].blockID = new_block_id;
    parent_dir.entries[parent_dir.nentries].fileindex = filename_index;
    parent_dir.nentries++;
    parent_dir.modtime = time(NULL);

    // write parent dir
    if (SIFS_writedirblock(vol, parent_dir_id, header, &parent_dir) != 0)
    {
        return 1;
    }


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

