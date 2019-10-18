//  CITS2002 Project 2 2019
//  Name(s):             Alexander Shearer, Thomas Kinsella
//  Student number(s):   22465777, 22177293

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sifs.h"

void usage(char *progname)
{
    fprintf(stderr, "Usage: %s volumename pathname\n", progname);
    fprintf(stderr, "or     %s pathname\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    char *volumename; // filename storing the SIFS volume
    char *pathname;   // the required directory name on the volume
    
    //  ATTEMPT TO OBTAIN THE volumename FROM AN ENVIRONMENT VARIABLE
    if (argcount == 2)
    {
        volumename = getenv("SIFS_VOLUME");
        if (volumename == NULL)
        {
            usage(argvalue[0]);
        }
        pathname = argvalue[1];
    }
    //  ... OR FROM A COMMAND-LINE PARAMETER
    else if (argcount == 3)
    {
        volumename = argvalue[1];
        pathname = argvalue[2];
    }
    else
    {
        usage(argvalue[0]);
        exit(EXIT_FAILURE);
    }
    
    //  VARIABLES TO STORE THE INFORMATION RETURNED FROM SIFS_dirinfo()
    size_t length;
    time_t modtime;
    
    //  PASS THE ADDRESS OF OUR VARIABLES SO THAT SIFS_dirinfo() MAY MODIFY THEM
    if (SIFS_fileinfo(volumename, pathname, &length, &modtime) != 0)
    {
        SIFS_perror(argvalue[0]);
        exit(EXIT_FAILURE);
    }
    
    //  Print info
    printf("modified %s\n", ctime(&modtime));
    printf("length %li\n", length);
    
    return EXIT_SUCCESS;
}


