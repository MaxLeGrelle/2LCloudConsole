#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#define BACKLOG 5

#define MSG_MAX 256
#define BLOCK_FILE_MAX 1024
#define NAME_FILE_MAX 255
#define PATH_FILE_MAX 255

#define EXEC -2
#define ADD -1

typedef struct {
  char fileData[BLOCK_FILE_MAX];
  char nameFile[NAME_FILE_MAX];
  char path[PATH_FILE_MAX];
  int size;
} File;

/* struct message used between server and client */
typedef struct {
  int numProg;
  File file;
  int code;
} StructMessage;

typedef struct {
  int numProg;
  int state;
  int timeOfExecution;
  int returnCode;
} MessageAfterExecution;

#endif
