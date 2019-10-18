//  CITS2002 Project 2 2019
//  Name(s):             Alexander Shearer, Thomas Kinsella
//  Student number(s):   22465777, 22177293

#include "sifs-internal.h"

// get information about a requested file
int SIFS_fileinfo(const char *volumename, const char *pathname,
		  size_t *length, time_t *modtime)
{
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

    // Cleanup
    fclose(vol);

    SIFS_freeparsedpath(parsed_path);
    parsed_path = NULL;
    
    SIFS_errno	= SIFS_EOK;
    return 0;
}

