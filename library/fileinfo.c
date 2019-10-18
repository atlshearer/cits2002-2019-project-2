#include "sifs-internal.h"

// get information about a requested file
int SIFS_fileinfo(const char *volumename, const char *pathname,
		  size_t *length, time_t *modtime)
{
    /*
     #define    SIFS_EINVAL    1    // Invalid argument
     #define    SIFS_ENOVOL    3    // No such volume
     #define    SIFS_ENOENT    4    // No such file or directory entry
     #define    SIFS_ENOTVOL    6    // Not a volume
     #define    SIFS_ENOTDIR    7    // Not a directory
     #define    SIFS_ENOTFILE    8    // Not a file
     */

    FILE *vol = fopen(volumename, "r");
    SIFS_VOLUME_HEADER header;
    char** parsed_path = NULL;
    size_t path_depth  = 0;

    SIFS_DIRBLOCK  parent;
    SIFS_FILEBLOCK target;

    SIFS_BLOCKID parent_id;
    SIFS_BLOCKID target_id;
    
    
    if (vol == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return 1;
    }
    
    SIFS_readheader(vol, &header);

    parsed_path = SIFS_parsepathname(pathname, &path_depth);
    
    if (SIFS_getfileblockid(vol, parsed_path, path_depth, header, &parent_id, &target_id) != 0) {
        return 1;
    }

    if (SIFS_readdirblock(vol, parent_id, header, &parent) != 0) {
        return 1;
    }

    if (SIFS_readfileblock(vol, target_id, header, &target) != 0) {
        return 1;
    }
    
    *length = target.length;
    *modtime = target.modtime;

    // cleanup
    fclose(vol);

    // Free parsed_path
    for (size_t i = 0; *(parsed_path + i); i++)
    {
        free(*(parsed_path + i));
    }
    free(parsed_path);
    
    SIFS_errno	= SIFS_EOK;
    return 0;
}

