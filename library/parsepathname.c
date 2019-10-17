#include "sifs-internal.h"

#include <string.h>

// parses a pathname string into an array of dir names
char** SIFS_parsepathname(const char *pathname, size_t* path_depth) {
    // implementation based on https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c


    char** parsed_path = NULL;
    size_t count       = 0;
    const char* tmp          = pathname; 
    char* pathname_cpy = malloc(strlen(pathname) + 1); // strtok is destructive

    if (pathname_cpy == NULL) {
        return NULL;
    } else {
        strcpy(pathname_cpy, pathname);
    }

    const char* last_slash   = 0;
    char delim[] = "/";




    while (*tmp)
    {
        if (delim[0] == *tmp)
        {
            count++;
            last_slash = tmp;
        }
        tmp++;
    }

    count += last_slash < (pathname + strlen(pathname) - 1);

    count++;

    parsed_path = malloc(sizeof(char*) * count);
    
    size_t idx = 0;
    if (parsed_path)
    {
        char* token = strtok(pathname_cpy, delim);

        while (token)
        {
            char* token_cpy = malloc(strlen(token) + 1);
            strcpy(token_cpy, token);
            *(parsed_path + idx) = token_cpy;
            idx++;
            token = strtok(NULL, delim);
        }
        *(parsed_path + idx) = 0;
    }

    *path_depth = idx;

    free(pathname_cpy);

    return parsed_path;
}

