#include <stdio.h>
#include <stdlib.h>
#include "sifs.h"

//  Written by Chris.McDonald@uwa.edu.au, September 2019

//  REPORT HOW THIS PROGRAM SHOULD BE INVOKED
void usage(char *progname)
{
    fprintf(stderr, "Usage: %s volumename pathname\n", progname);
    fprintf(stderr, "or     %s pathname\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    char *volumename; // filename storing the SIFS volume
    char *pathname;   

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

    //  ATTEMPT TO CREATE THE NEW VOLUME
    if (SIFS_mkdir(volumename, pathname) != 0)
    {
        SIFS_perror(argvalue[0]);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
