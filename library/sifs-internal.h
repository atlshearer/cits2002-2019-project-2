//  CITS2002 Project 2 2019
//  Name(s):             Alexander Shearer, Thomas Kinsella
//  Student number(s):   22465777, 22177293

#include "../sifs.h"
#include "md5.h"

#include <stdio.h>

//  CONCRETE STRUCTURES AND CONSTANTS USED THROUGHOUT THE SIFS LIBRARY.
//  DO NOT CHANGE ANYTHING IN THIS FILE.

#define SIFS_MIN_BLOCKSIZE 1024

#define SIFS_MAX_NAME_LENGTH 32 // including the NULL byte
#define SIFS_MAX_ENTRIES 24     // for both directory and file entries

#define SIFS_UNUSED 'u'
#define SIFS_DIR 'd'
#define SIFS_FILE 'f'
#define SIFS_DATABLOCK 'b'

#define SIFS_ROOTDIR_BLOCKID 0

typedef struct
{
    size_t blocksize;
    uint32_t nblocks;
} SIFS_VOLUME_HEADER;

typedef char SIFS_BIT; // SIFS_UNUSED, SIFS_DIR, ...
typedef uint32_t SIFS_BLOCKID;

//  DEFINITION OF EACH DIRECTORY BLOCK - MUST FIT INSIDE A SINGLE BLOCK
typedef struct
{
    char name[SIFS_MAX_NAME_LENGTH];
    time_t modtime; // time last modified <- time()

    uint32_t nentries;
    struct
    {
        SIFS_BLOCKID blockID; // of the entry's subdirectory or file
        uint32_t fileindex;   // into a SIFS_FILEBLOCK's filenames[]
    } entries[SIFS_MAX_ENTRIES];
} SIFS_DIRBLOCK;

//  DEFINITION OF EACH FILE BLOCK - MUST FIT INSIDE A SINGLE BLOCK
typedef struct
{
    time_t modtime; // time first file added <- time()
    size_t length;  // length of files' contents in bytes

    unsigned char md5[MD5_BYTELEN];
    SIFS_BLOCKID firstblockID;

    uint32_t nfiles; // n files with identical contents
    char filenames[SIFS_MAX_ENTRIES][SIFS_MAX_NAME_LENGTH];
} SIFS_FILEBLOCK;

// PRIVATE HELPER FUNCTIONS ----------------------------------------

// parses a pathname string into an array of dir names
extern  int SIFS_parsepathname(const char *pathname, char*** parsed_path, size_t* path_depth);

// frees all memory allocated by SIFS_parsepathname
extern  void SIFS_freeparsedpath(char** parsed_path);


// traversal helpers -----------------

// checks if the opened file is a valid volume
//      - Header has valid values
//      - File is of correct size
//      - Bitmap contains only valid bits
//      - File and Directory blocks contain valid values
//      - File and Directory blocks 'point' to correct block type
//      - DOES NOT CHECK for structure issues, e.g. directory loops
extern  int SIFS_checkvolumeintegrity(FILE* volume, SIFS_VOLUME_HEADER vol_header);

// gets file block and parent id block by path name
extern  int SIFS_getfileblockid(FILE* volume, char **parsed_path, size_t path_depth, SIFS_VOLUME_HEADER vol_header, SIFS_BLOCKID* parent, SIFS_BLOCKID* target);

// gets dir block id by path name
extern  int SIFS_getdirblockid(FILE* volume, char **parsed_path, size_t path_depth, SIFS_VOLUME_HEADER vol_header, SIFS_BLOCKID* target);

// sets SIFS_errno to SIFS_EEXIST and retunrs 1 if exists, else returns 0
extern  int SIFS_checkexists(FILE* volume, SIFS_DIRBLOCK parent_id, char* target_name, SIFS_VOLUME_HEADER vol_header);


// reader helpers ---------------------

// get header and store in vol_header
// also checks if volume is of correct size, with valid bitmap
extern  int SIFS_readheader(FILE* volume, SIFS_VOLUME_HEADER* vol_header);

// get dir  block located at block_id and store in  dir_block
extern  int SIFS_readdirblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_DIRBLOCK* dir_block);
                              
// get file block located at block_id and store in file_block
extern  int SIFS_readfileblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_FILEBLOCK* file_block);

// get data starting at block located block_id
extern  int SIFS_readdata(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, void* data, size_t nbytes);

// get the type of block located at block_id and store in block_type
extern  int SIFS_readblocktype(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_BIT* block_type);


// writter helpers ---------------------

// write dir_block to block at location block_id
extern  int SIFS_writedirblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_DIRBLOCK* dir_block);

// write file_block to block at location block_id
extern  int SIFS_writefileblock(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_FILEBLOCK* file_block);

// write data starting at block located block_id
extern  int SIFS_writedata(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, void* data, size_t nbytes);

// write block_type to block at location block_id
extern  int SIFS_writeblocktype(FILE* volume, SIFS_BLOCKID block_id, SIFS_VOLUME_HEADER vol_header, SIFS_BIT* block_type, size_t nblocks);



