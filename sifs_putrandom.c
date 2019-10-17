#include <stdio.h>
#include <stdlib.h>
#include "sifs.h"

//  Written by Chris.McDonald@uwa.edu.au, September 2019

//  REPORT HOW THIS PROGRAM SHOULD BE INVOKED
void usage(char *progname)
{
    fprintf(stderr, "Usage: %s volumename pathname datelen\n", progname);
    fprintf(stderr, "or     %s pathname datelen\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    char *volumename; // filename storing the SIFS volume
    char *pathname;
    char *data;
    int data_len;

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    //  ATTEMPT TO OBTAIN THE volumename FROM AN ENVIRONMENT VARIABLE
    if (argcount == 3)
    {
        volumename = getenv("SIFS_VOLUME");
        if (volumename == NULL)
        {
            usage(argvalue[0]);
        }
        pathname = argvalue[1];
        data_len = atoi(argvalue[2]);
    }
    //  ... OR FROM A COMMAND-LINE PARAMETER
    else if (argcount == 4)
    {
        volumename = argvalue[1];
        pathname = argvalue[2];
        data_len = atoi(argvalue[3]);
    }
    else
    {
        usage(argvalue[0]);
        exit(EXIT_FAILURE);
    }

    data = malloc(sizeof(char) * data_len);

    if (!data)
    {
        SIFS_errno = SIFS_ENOMEM;
        SIFS_perror(argvalue[0]);
        return EXIT_FAILURE;
    }
    

    for (size_t i = 0; i < data_len; i++)
    {
        data[i] = charset[rand() % (int)(sizeof(charset) - 1)];
    }
    
    //  ATTEMPT TO CREATE THE NEW VOLUME
    if (SIFS_writefile(volumename, pathname, data, data_len) != 0)
    {
        SIFS_perror(argvalue[0]);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

