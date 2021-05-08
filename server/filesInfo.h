#ifndef _FILESINFO_H_
#define _FILESINFO_H_

#define MAX_NAME 		50
#define MAX_FILES       1000

/* struct file informations used to get informations of a specific file*/
typedef struct {
    int number;
    char name[MAX_NAME];
    int numberOfExecutions;
    int totalTimeExecution;
} fileInfo;

/* files used to tidy file informations*/
typedef struct {
    int size;
    fileInfo tab[MAX_FILES];
} files;
#endif