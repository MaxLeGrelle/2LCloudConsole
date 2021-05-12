#ifndef _SERVERUTIL_H_
#define _SERVERUTIL_H_

#include <stdbool.h>

#define MAX_NAME 50
#define MAX_FILES 1000
#define SHM_KEY 493 //shared memory key
#define SEM_KEY 925 //semaphore key

/* struct file informations used to get informations of a specific file*/
typedef struct {
    int number;
    char name[MAX_NAME];
    bool compile;
    int numberOfExecutions;
    int totalTimeExecution;

} fileInfo;

/* files used to tidy file informations*/
typedef struct {
    int size;
    fileInfo tab[MAX_FILES];
} files;
#endif