#include "sifs-internal.h"

// get information about a requested file
int SIFS_fileinfo(const char *volumename, const char *pathname,
		  size_t *length, time_t *modtime)
{
    SIFS_errno	= SIFS_ENOTYET;
    return 1;
}
