#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "../utils_v10.h"
#include "serverUtil.h"

#define MAX_CLIENTS 50
#define DEFAULTNAME_OUTPUTFILE 0
#define MAX_PATH_PROG 25

static int outputNum = DEFAULTNAME_OUTPUTFILE;
static int sem_id;
static int shm_id;

/**
 * PRE: numProg : a positive integer.
 * RES: True if numProg is the number of a program contained in the shared memory,
 * false otherwise.
 */
bool containedInSharedMemory(int numProg);

/**
 * RES: the next number for a new program.
 */
int getNewNumProg();

/**
 * PRE: compiled : integer which determines if the program number numProg compiled.
 * if the program compiled, the parameter equals 0, else an integer different to 0.
 *      name : the name of the program number numProg.
 *      numProg : the number of the program recently added.
 * POST: It adds all the information related to the new program into the shared memory
 * and increase its size by one.
 */
void addProgInfoToSharedMem(int compiled, char* name, int numProg);

/**
 * PRE: compiled : integer which determines if the program number numProg compiled.
 * if the program compiled, the parameter equals 0, else an integer different to 0.
 *      name : the name of the program number numProg.
 *      numProg : the number of the program to edit.
 * POST: It edits all the information related to the program from the shared memory
 */
void editProgInfoInSharedMem(int compiled, char* name, int numProg);

/**
 * PRE: arg1 : a valid string containing the path of the program to compile.
 * POST: Tries to compile the program located at arg1 thanks to an execl.
 */
void compile(void* arg1);

/**
 * PRE: socketClient : file descriptor of the client who made the query to add/edit a program.
 *      fd : file descriptor of the file to add/edit in the code repository.
 * POST: Reads the file received from the client and write its content to file descriptor fd.
 */
void readClientsFileAndWriteToFile(int socketClient, int fd);

/**
 * PRE: path : a valid string which is the path of a program C.
 * POST: It creates an output file and writes the output of the compilation of the program C in it.
 */
int writeOutputFileAndCompile(char* path);

/**
 * PRE: message : contains all information about the request from the client.
 *      socket : file descriptor of the client's socket.
 * POST: It allows to edit an existing program by re writing all the content of the old file
 * by the new one and compiles it. The output of the compilation will be wrote in a new output file.
 * RES: Returns a struct ReturnMessage which contains the response of the server that will be sent to the client.
 */
ReturnMessage editProgram(StructMessage message, int socket);

/**
 * PRE: fileToCreate : Is the new file to create.
 *      socket : file descriptor of the client's socket.
 * POST: It allows to add and compile a new program. The output of the compilation will be wrote in a new output file.
 * RES: Returns a struct ReturnMessage which contains the response of the server that will be sent to the client.
 */
ReturnMessage addProgram(File fileToCreate, int socket);

/**
 * PRE: returnMessage : contains all the information that will be send to the client.
 *      clientSocket : file descriptor of the client's socket.
 * POST: It first sends the content of returnMessage to the client then it will read and
 * write the content of the output file to the client.
 */
void sendResponseToClient(const ReturnMessage* returnMessage, int clientSocket);

/**
 * PRE: socket : file descriptor of the client's socket.
 * POST: Allows to call the appropriate method related to the action requested by the client.
 */
void clientProcess(void* socket);

/**
 * POST: a new file is created and open (ready to be used).
 * RES: the file descriptor of this new file
 */
int createAndOpenOutputFile();

/**
 * PRE: arg1 & arg2 : strings to execute execl
 *      fd : file descriptor to get the output
 * POST: a program is executed and the output is in the file given
 */
void execution(void* arg1, void* arg2, void* fd);

/**
 * PRE: numProg : the number of the program executed
 *      timeSpent : the time of execution of the program
 * POST: the shared memory is updated according to the execution
 */
void changeInformationAfterExec(int numProg, long timeSpent);

/**
 * PRE: numProg: the number of the program to execute
 * RES: return the status according to informations in shared memory 
 */
int stateCheck(int numProg);

/**
 * PRE: numProg : the number of the program to execute
 * POST: the program is executed if it exist and compile
 * RES: a message with the informations of the execution
 */
ReturnMessage execProgram(int numProg);

/**
 * PRE: clientSOcketFD : the file descriptor of the client socket
 * POST: the output is send to the client by the socket
 */
void writeOutput(int* clientSocketFD);

bool containedInSharedMemory(int numProg) {
    files* f = sshmat(shm_id);
    bool contained = numProg <= f->size-1 && numProg >= 0;
    sshmdt(f);
    return contained; 
}

int createAndOpenOutputFile(){
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

    FileInfo fileinfo;
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

void editProgInfoInSharedMem(int compiled, char* name, int numProg) {
    files* f = sshmat(shm_id);
    sem_down0(sem_id);

    FileInfo fileInfo = f->tab[numProg];
    fileInfo.compile = compiled;
    fileInfo.numberOfExecutions = 0;
    fileInfo.totalTimeExecution = 0;
    strcpy(fileInfo.name, name);
    f->tab[numProg] = fileInfo;

    sshmdt(f);
    sem_up0(sem_id);
}

void compile(void* arg1) {
    char* path = arg1;
    char* executable = strdup(path);
    executable[strlen(executable)-2] = '\0'; //remove .c
    sexecl("/usr/bin/gcc", "cc", "-o", executable, path, NULL);
}

void readClientsFileAndWriteToFile(int socketClient, int fd) {
    char fileBlock[BLOCK_FILE_MAX];
    printf("En attente du contenu du fichier à ajouter ...\n");
    int nbCharLu = sread(socketClient, fileBlock, BLOCK_FILE_MAX);
    while(nbCharLu != 0) {
        nwrite(fd, fileBlock, nbCharLu);
        nbCharLu = sread(socketClient, fileBlock, BLOCK_FILE_MAX);
    }
    printf("Tout le fichier a été recu !\n");
}

int writeOutputFileAndCompile(char* path) {
    //creation fichier de sortie
    int fdOut = createAndOpenOutputFile();
    printf("Fichier de sortie: %d\n", outputNum);

    //redirection sortie erreur vers le fichier out.
    int fdStderr = dup(2);
    dup2(fdOut, 2);

    //compilation du fichier
    int childPID = fork_and_run1(compile, path);
    swaitpid(childPID, NULL, 0);
    //remise de stderr en 2
    dup2(fdStderr, 2);

    char pathOutput[MAX_PATH_PROG];
    sprintf(pathOutput, "output/%d", outputNum);
    fdOut = sopen(pathOutput, O_RDONLY, 0600);

    //lecture fichier out pour savoir si le prog recu a compilé
    char msgCompilationErr[1];
    int nbCharLu = sread(fdOut, msgCompilationErr,1);
    int compiled = 0;
    if (nbCharLu != 0) compiled = 1;

    sclose(fdOut);

    return compiled;
}

ReturnMessage editProgram(StructMessage message, int socket) {
    ReturnMessage returnMessage;
    returnMessage.numProg = message.numProg;

    printf("Le client veut modifier le programme numéro: %d\n", returnMessage.numProg);

    //ouverture du fichier à remplacer
    char path[MAX_PATH_PROG];
    sprintf(path, "programs/%d.c", returnMessage.numProg);
    int fdReplaceFile = sopen(path, O_WRONLY | O_TRUNC, 0600);

    //lecture du nouveau programme qui va en remplacer un venant du client
    readClientsFileAndWriteToFile(socket, fdReplaceFile);

    int compiled = writeOutputFileAndCompile(path);

    editProgInfoInSharedMem(compiled, message.file.nameFile.name, returnMessage.numProg);
    returnMessage.compile = compiled;

    return returnMessage;
}

ReturnMessage addProgram(File fileToCreate, int socket) {
    ReturnMessage returnMessage;
    int numProg = getNewNumProg();
    returnMessage.numProg = numProg;

    printf("Le client veut ajouter un programme\n");

    //création du fichier (vide) qui va contenir le programme à ajouter
    char path[MAX_PATH_PROG];
    sprintf(path, "programs/%d.c",numProg);
    int fdNewFile = sopen(path, O_CREAT | O_WRONLY, 0777);

    //lecture du fichier à ajouter venant du client.
    readClientsFileAndWriteToFile(socket, fdNewFile);

    int compiled = writeOutputFileAndCompile(path);

    addProgInfoToSharedMem(compiled, fileToCreate.nameFile.name, returnMessage.numProg);
    returnMessage.compile = compiled;
    
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

ReturnMessage execProgram(int numProg) {
    ReturnMessage mae;
    mae.numProg = numProg;
    
    printf("Le client veut exec le prog num: %d\n", numProg);

    mae.state = stateCheck(numProg);
    if(mae.state != 1){
        if (mae.state == -1) {
            printf("Le programme ne compile pas !\n");
        }
        return mae;
    }

    int fd  = createAndOpenOutputFile();
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

void sendResponseToClient(const ReturnMessage* returnMessage, int clientSocket) {
    swrite(clientSocket, returnMessage, sizeof(*returnMessage));
    writeOutput(&clientSocket);
}

//thread lié à un client
void clientProcess(void* socket) {
    ReturnMessage retM;
    int* clientSocketFD = socket;
    StructMessage message;
    sread(*clientSocketFD, &message, sizeof(message));
    if (message.code == ADD) {
        retM = addProgram(message.file, *clientSocketFD);
        sendResponseToClient(&retM, *clientSocketFD);
    }else if (message.code == EXEC) {
        retM = execProgram(message.numProg);
        sendResponseToClient(&retM, *clientSocketFD);
    }else if (containedInSharedMemory(message.numProg)) {
        retM = editProgram(message, *clientSocketFD);
        sendResponseToClient(&retM, *clientSocketFD);
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