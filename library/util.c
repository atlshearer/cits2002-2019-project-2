//  CITS2002 Project 2 2019
//  Name(s):             Alexander Shearer, Thomas Kinsella
//  Student number(s):   22465777, 22177293

#include "sifs-internal.h"

#include <string.h>

int SIFS_parsepathname(const char *pathname, char*** parsed_path, size_t* path_depth) {
    // implementation based on https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c

    size_t count = 0;
    const char* tmp = pathname; 
    char* pathname_cpy = malloc(strlen(pathname) + 1); // strtok is destructive

    if (pathname_cpy == NULL) 
    {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }
        
    strcpy(pathname_cpy, pathname);


    const char* last_slash   = 0;
    char delim[] = "/";

    while (*tmp)
    {
        if (delim[0] == *tmp)
        {
            count++;
            last_slash = tmp;

            if (delim[0] == *(tmp + 1)) 
            {
                // Two / next to eachother invalid filepath
                SIFS_errno = SIFS_EINVAL;
                return 1;
            }
        }
        tmp++;
    }

    count += last_slash < (pathname + strlen(pathname) - 1);

    count++;

    *parsed_path = malloc(sizeof(char*) * count);

    if (*parsed_path == NULL) {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }
    
    size_t idx = 0;
    if (*parsed_path)
    {
        char* token = strtok(pathname_cpy, delim);

        while (token)
        {
            if (strlen(token) + 1 > SIFS_MAX_NAME_LENGTH) {
                SIFS_errno = SIFS_EINVAL;
                return 1;
            }
            

            char* token_cpy = malloc(strlen(token) + 1);

            if (token_cpy == NULL) {
                SIFS_errno = SIFS_ENOMEM;
                return 1;
            }

            strcpy(token_cpy, token);
            *(*parsed_path + idx) = token_cpy;
            idx++;
            token = strtok(NULL, delim);
        }
        *(*parsed_path + idx) = 0;
    }

    *path_depth = idx;

    free(pathname_cpy);

    return 0;
}

void SIFS_freeparsedpath(char** parsed_path) {
    for (size_t i = 0; *(parsed_path + i); i++) {
        free(*(parsed_path + i));
        *(parsed_path + i) = NULL;
    }
}