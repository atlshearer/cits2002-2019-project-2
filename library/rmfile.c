#include "sifs-internal.h"

#include <stdio.h>
#include <string.h>

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
    
    FILE *vol = fopen(volumename, "r");
    SIFS_VOLUME_HEADER header;
    char** parsed_path = NULL;
    size_t path_depth  = 0;
    
    SIFS_DIRBLOCK  parent;
    SIFS_FILEBLOCK target;
    
    SIFS_BLOCKID parent_id;
    SIFS_BLOCKID target_id;
    
    SIFS_BIT out_buffer;
    
    
    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }
    
    SIFS_getheader(vol, &header);
    
    parsed_path = SIFS_parsepathname(pathname, &path_depth);
    
    if (SIFS_getfileblockid(vol, parsed_path, path_depth, header, &parent_id, &target_id) != 0) {
        return 1;
    }
    
    if (SIFS_getdirblock(vol, parent_id, header, &parent) != 0) {
        return 1;
    }
    
    if (SIFS_getfileblock(vol, target_id, header, &target) != 0) {
        return 1;
    }
    
    
    // find target index
    for (size_t i = 0; i < parent.nentries; i++) {
        if (parent.entries[i].blockID == target_id) {
            // Check if other copies exist
            if (target.nfiles > 1) {
                // Remove filename from target
                for (size_t j = parent.entries[i].fileindex; j < target.nfiles - 1; j++) {
                    strcpy(target.filenames[j], target.filenames[j + 1]);
                }
                
                target.nfiles--;
                
                // Write back to file
                SIFS_writefileblock(vol, target_id, header, &target);
            } else {
                // Remove file from archive
                out_buffer = SIFS_UNUSED;
                SIFS_writeblocktype(vol, target_id, header, &out_buffer, 1);
                SIFS_writeblocktype(vol, target.firstblockID, header, &out_buffer, target.length);
            }
            
            // Remove reference in parent
            for (size_t j = i; j < parent.nentries - 1; j++) {
                parent.entries[j] = parent.entries[j + 1];
            }
            
            parent.nentries--;
            
            // Write back to file
            SIFS_writedirblock(vol, parent_id, header, &parent);
        }
    }
    
    
    SIFS_errno	= SIFS_ENOTYET;
    return 1;
}

