#ifndef __LibFS_h__
#define __LibFS_h__

/*
 * DO NOT MODIFY THIS FILE
 */
    
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// used for errors
extern int osErrno;
 
#define NO_OF_INODES 1000 
#define NO_OF_FILEDESCRIPTORS 32  //No of File descriptors can be initialized here
#define FS fileDescriptor_map[fildes].second    

// error types - don't change anything about these!! (even the order!)
typedef enum {
    E_GENERAL,      // general
    E_CREATE, 
    E_NO_SUCH_FILE, 
    E_TOO_MANY_OPEN_FILES, 
    E_BAD_FD, 
    E_NO_SPACE, 
    E_FILE_TOO_BIG, 
    E_SEEK_OUT_OF_BOUNDS, 
    E_FILE_IN_USE, 
    E_BUFFER_TOO_SMALL, 
    E_DIR_NOT_EMPTY,
    E_ROOT_DIR,
} FS_Error_t;
    
// File system generic call
int FS_Boot(char *path);
int FS_Sync();

// file ops
int File_Create(char *file);
int File_Open(char *file);
int File_Read(int fd, void *buffer, int size);
int File_Write(int fd, char *buffer, int size);
int File_Seek(int fd, int offset);
int File_Close(int fd);
int File_Unlink(char *file);
//we added mounting function
int mounting(); //taking metadata into data Structure

// directory ops
int Dir_Create(char *path);
int Dir_Size(char *path);
int Dir_Read(char *path, void *buffer, int size);
int Dir_Unlink(char *path);

int store_file_into_Disk(char *);

int unmounting(void);







#endif /* __LibFS_h__ */




