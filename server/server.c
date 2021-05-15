#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "../utils_v10.h"
#include "serverUtil.h"

#define MAX_CLIENTS 50
#define DEFAULTNAME_OUTPUTFILE 0

static int outputNum = DEFAULTNAME_OUTPUTFILE;
static int sem_id;
static int shm_id;

int createAndOpenReturnFile(){
    char arg[25];
    sprintf(arg, "output/%d",outputNum);
    int fd = sopen(arg, O_RDWR | O_TRUNC | O_CREAT, 0644);
    return fd;
}

int getNewNumProg() {
    int numProg = -1;
    //semaphores ?
    files* f = sshmat(shm_id);
    numProg = f->size;
    sshmdt(f);

    return numProg;
}

void addProgInfoToSharedMem(int compiled, char* name, int numProg) {
    files* f = sshmat(shm_id);

    sem_down0(sem_id);
    fileInfo fileinfo;
    fileinfo.compile = compiled;
    fileinfo.numberOfExecutions = 0;
    fileinfo.totalTimeExecution = 0;
    fileinfo.number = numProg;
    strcpy(fileinfo.name, name);
    f->tab[fileinfo.number] = fileinfo;
    f->size++;
    sshmdt(f);
    sem_up0(sem_id);
}

void compile(void* arg1) {
    char* path = arg1;
    char* executable = strdup(path);
    executable[strlen(executable)-2] = '\0'; //remove .c
    sexecl("/usr/bin/gcc", "cc", "-o", executable, path, NULL);
}

ReturnMessage addProgram(File fileToCreate, int socket) {
    ReturnMessage returnMessage;
    int numProg = getNewNumProg();
    returnMessage.numProg = numProg;

    printf("Le client veut ajouter un programme\n");
    char path[25];
    sprintf(path, "programs/%d.c",numProg);

    //lecture du fichier à ajouter venant du client.
    int fdNewFile = sopen(path, O_CREAT | O_WRONLY, 0777);
    char fileBlock[BLOCK_FILE_MAX];
    printf("En attente du contenu du fichier à ajouter ...\n");
    int nbCharLu = sread(socket, fileBlock, BLOCK_FILE_MAX);
    while(nbCharLu != 0) {
        nwrite(fdNewFile, fileBlock, nbCharLu);
        nbCharLu = sread(socket, fileBlock, BLOCK_FILE_MAX);
    }

    //creation fichier de sortie
    int fdOut = createAndOpenReturnFile();
    printf("Fichier de sortie: %d\n", outputNum);

    //redirection sortie erreur vers le fichier out.
    int fdStderr = dup(2);
    dup2(fdOut, 2);

    //compilation du fichier
    int childPID = fork_and_run1(compile, path);
    swaitpid(childPID, NULL, 0);

    //remise de stderr en 2
    dup2(fdStderr, 2);

    //lecture fichier out pour savoir si le prog recu a compilé
    char msgCompilationErr[1];
    nbCharLu = sread(fdOut, msgCompilationErr, 1);
    int compiled = 0;
    if (nbCharLu != 0) compiled = 1;
    addProgInfoToSharedMem(compiled, fileToCreate.nameFile, numProg);
    returnMessage.compile = compiled;

    printf("Tout le fichier a été recu !\n");
    

    sclose(fdOut);
    return returnMessage;
}

void execution(void* arg1, void* arg2, void* fd){
    int* arg = fd;
    dup2(*arg,1);
    char* a1 = arg1;
    char* a2 = arg2;
    sexecl(a1, a2,NULL);
}

void changeInformationAfterExec(int numProg, long timeSpent){
    // GET SHARED MEMORY
    files* f = sshmat(shm_id);

    sem_down0(sem_id);
    f->tab[numProg].numberOfExecutions++;
    f->tab[numProg].totalTimeExecution += timeSpent;
    sshmdt(f);
    sem_up0(sem_id); 

}

int stateCheck(int numProg){
    int ret = 1;
    files* f = sshmat(shm_id);

    sem_down0(sem_id);
    if(f->size <= numProg) ret = -2;
    else if(f->tab[numProg].compile != 0) ret = -1;
    sshmdt(f);
    sem_up0(sem_id);
    return ret;
}

ReturnMessage execProgram(int numProg, void* socket) {
    ReturnMessage mae;
    mae.numProg = numProg;
    
    printf("Le client veut exec le prog num: %d\n", numProg);

    mae.state = stateCheck(numProg);
    if(mae.state != 1){
        if (mae.state == -1) {
            printf("Le programme ne compile pas !\n");
        }
        //mae.returnCode = -1;
        //mae.timeOfExecution = 0;
        return mae;
    }

    int fd  = createAndOpenReturnFile();
    struct timeval start, end;
    char arg1[25];
    char arg2[25];
    sprintf(arg1, "./programs/%d",numProg);
    sprintf(arg2, "programs/%d",numProg);

    gettimeofday(&start, NULL);

    int childId = fork_and_run3(execution, &arg1, &arg2, &fd);
    swaitpid(childId, &mae.returnCode, 0);
    if(mae.returnCode != 0) mae.state = 0;

    gettimeofday(&end, NULL);
    long timeSpentSeconds = (end.tv_sec - start.tv_sec);
    long timeSpentMicro = (end.tv_usec - start.tv_usec);
    mae.timeOfExecution = timeSpentSeconds*1000000 + timeSpentMicro;
    
    changeInformationAfterExec(numProg, mae.timeOfExecution);
    sclose(fd);
    return mae;
}

void writeOutput(int* clientSocketFD){
    char arg[20];
    sprintf(arg, "output/%d", outputNum);
    int fd = sopen(arg, O_RDONLY, 0600);
    char buff[OUTPUT_MAX];
    int nbCharLu = sread(fd, buff, OUTPUT_MAX);
    while(nbCharLu != 0) {
        nwrite(*clientSocketFD, buff, nbCharLu);
        nbCharLu = sread(fd, buff, OUTPUT_MAX);
    }
    sclose(fd);
    int ret = shutdown(*clientSocketFD, SHUT_WR);
    checkNeg(ret, "ERROR shutdown\n");
}

//thread lié à un client
void clientProcess(void* socket) {
    ReturnMessage retM;
    int* clientSocketFD = socket;
    StructMessage message;
    sread(*clientSocketFD, &message, sizeof(message));
    if (message.code == ADD) {
        retM = addProgram(message.file, *clientSocketFD);
        swrite(*clientSocketFD, &retM, sizeof(ReturnMessage));
        writeOutput(clientSocketFD);
    }else if (message.code == EXEC) {
        retM = execProgram(message.numProg, socket);
        swrite(*clientSocketFD, &retM, sizeof(ReturnMessage));
        writeOutput(clientSocketFD);
    }
    
    
}

int main(int argc, char* argv[]) {
    
    int port = atoi(argv[1]);
    int clientSocketFD;

    //init server's socket
    int servSockFD = initSocketServer(port);
    printf("Le serveur est à l'écoute sur le port %d\n", port);

    // GET SEMAPHORE
    sem_id = sem_get(SEM_KEY, 1);

    // GET SHARED MEMORY
    shm_id = sshmget(SHM_KEY, sizeof(files), 0);

    //listening if a client want to connect or communicate with the server.
    while(true) {
        clientSocketFD = saccept(servSockFD);
        outputNum ++;
        printf("%d) Une demande à été reçue !\n", outputNum);
        
        //création d'un enfant par client qui se connecte.
        fork_and_run1(clientProcess, &clientSocketFD);
    }
}