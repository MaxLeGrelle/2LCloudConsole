#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

#include "../utils_v10.h"

#define COMM_ADD_PROG '+'
#define COMM_EDIT_PROG '.'
#define COMM_EXECREC_PROG '*'
#define COMM_EXEC_PROG '@'
#define COMM_EXIT 'q'
#define COMM_HELP 'h'
#define HEARTBEAT -1
#define NBRMAXPROGRAM 100

#define MAX_COMM 255

char* serverIp;
int serverPort;

/**
 * POST: prints on stdout the list of commands available.
 */
void printHelp();

/**
 * PRE: messageToSend : is a StructMessage which is use to send 
 * information about the program to add/edit to the server.
 *     socketServer : file descriptor of the server's socket.
 * POST: messageToSend is send to the server thanks to its socket socketServer,
 * and the file located at the path in messageToSend is read and send to the server.
 * Finally, it waits for the server's response.
 */
void askServerAddOrEditProgram(const StructMessage* messageToSend, int socketServer);

/**
 * PRE: socketServer : file descriptor of the server's socket.
 *      interval : number of seconds to wait before each execution 
 * when the user wants to perform recurrent execution.
 * POST: reads stdin for a command and call the appropriated method.
 * If the command entered is 'q' then the socket to the server 
 * will be closed and the program will exits.
 * If the command entered is not known, then it print the list of commands available.
 */
void readCommandUser(int socketServer, int interval);



/**
 * PRE: socketServer : file descriptor of the server socket
 *      request : what is the type of message
 * POST: print the message received by the socket
 */
void readServerResponse(int socketServer, int request);

/**
 * PRE: messageToReturn : the message to write on the socket
 *      numProg : the num of the prog to execute
 *      socketServer :  file descriptor of the server socket
 * POST: write the message to send informations to the server
 */ 
void askServerExecProgram(StructMessage messageToReturn, int numProg, int socketServer);

/**
 * PRE: sockFd : file descriptor of the server socket
 *      pipe: file descriptor of a pipe (use only to read)
 * POST: execute files added to the tab or add a num of prog to the tab (according to the pipe)
 */
void recurrentExec(void* sockFd, void* pipe);

/**
 * PRE: interval : interval of time to sleep
 *      pipe: file descriptor of a pipe (use only to write)
 * POST: send a HEARTBEAT (-1) in the pipe all interval of time
 */
void timer(void* interval, void* pipe);

/**
 * PRE: interval : interval of time
 *      sockfd: file descriptor of the server socket
 * POST: create a pipe and init the timer and the recurentExec
 * RES: the pipe(write side only)
 */
int initRecExecution(int interval, int sockfd);

void readServerResponse(int socketServer, int request){
    ReturnMessage retM;
    sread(socketServer, &retM, sizeof(ReturnMessage));

    printf("numéro de programme: %d\n",retM.numProg);
    char* titleOutput = "output:";
    if (request != EXEC) {
        if(retM.compile == 0){
            printf("Le programme compile\n");
        }else{
            printf("Le programme ne compile pas\n");
            titleOutput = "Message d'erreur du compilateur:";
        }
    }else {
        printf("état du programme: %d\n",retM.state);
        printf("temps d'exécution du programme: %d\n",retM.timeOfExecution);
        printf("code de retour: %d\n",retM.returnCode);
    }
    
    if (retM.state != -1) {
        printf("%s\n", titleOutput);
        char buff[OUTPUT_MAX];
        int nbCharLu = sread(socketServer, buff, OUTPUT_MAX);
        while(nbCharLu != 0) {
            nwrite(0, buff, nbCharLu);
            nbCharLu = sread(socketServer, buff, OUTPUT_MAX);
        }
    }
    
}

void askServerExecProgram(StructMessage messageToReturn, int numProg, int socketServer){
    messageToReturn.code = EXEC;
    messageToReturn.numProg = numProg;
    swrite(socketServer, &messageToReturn, sizeof(messageToReturn));
    readServerResponse(socketServer, EXEC);
}

void recurrentExec(void* sockFd, void* pipe){
    int tab[NBRMAXPROGRAM];
    int size = 0;
    int* socketServer = sockFd;
    int* pipefd = pipe;
    //fermeture de l'écriture
    sclose(pipefd[1]);

    int intRead;
    while(true){
        sread(pipefd[0], &intRead, sizeof(int));
        if(intRead == HEARTBEAT){
            //execute all the tab
            for(int i=0; i<size; i++){
                StructMessage messageToReturn;
                askServerExecProgram(messageToReturn, tab[i], *socketServer);
                *socketServer = initSocketClient(serverIp, serverPort);
            }
        }else{
            //add the file given to the tab
            tab[size] = intRead;
            size++;
        }
    }
}

void printHelp() {
    printf("Commandes disponibles:\n");
    printf("\t- %c: <chemin d'un fichier C>: Permet d'ajouter le programme situé au chemin spécifié\n", COMM_ADD_PROG);
    printf("\t- %c: num <chemin d'un fichier C>: Permet de remplacer le programme associé portant le numéro 'num'\n", COMM_EDIT_PROG);
    printf("\t- %c: num: Permet de demander au serveur d'exécuter de manière récurrente le programme ayant le numéro 'num'\n", COMM_EXECREC_PROG);
    printf("\t- %c: num: Permet de demander au serveur d'exécuter le programme ayant le numéro 'num'\n", COMM_EXEC_PROG);
    printf("\t- %c: Permet d'afficher l'aide\n", COMM_HELP);
    printf("\t- %c: Quitte le programme\n", COMM_EXIT);
}

void askServerAddOrEditProgram(const StructMessage* messageToSend, int socketServer) {
    //envoie des informations concernant l'opération à effectuer au server
    swrite(socketServer, messageToSend, sizeof(*messageToSend));
    //envoie fichier vers server
    int fd = sopen(messageToSend->file.path, O_RDONLY | O_CREAT, 0600);
    char buff[BLOCK_FILE_MAX];
    int nbCharLu = sread(fd, buff, BLOCK_FILE_MAX);
    while(nbCharLu != 0) {
        nwrite(socketServer, buff, nbCharLu);
        nbCharLu = sread(fd, buff, BLOCK_FILE_MAX);
    }
    int ret = shutdown(socketServer, SHUT_WR);
    checkNeg(ret, "ERROR shutdown\n");

    //attente réponse server
    readServerResponse(socketServer, messageToSend->numProg);
}

void timer(void* interval, void* pipe){
    int* intervalValue = interval;
    int* pipefd = pipe;
    int beat = HEARTBEAT;
    while(true){
        sleep(*intervalValue);
        //ecrire -1 sur le pipe
        swrite(pipefd[1], &beat, sizeof(int));
    }
}

int initRecExecution(int interval, int sockfd){
    int pipefd[2];
    //création du pipe
    spipe(pipefd);
    fork_and_run2(recurrentExec, &sockfd, &pipefd);
    //fermeture de la lecture sur le pipe
    sclose(pipefd[0]);
    fork_and_run2(timer, &interval, &pipefd);
    return pipefd[1];
}

void readCommandUser(int socketServer, int interval) {
    StructMessage messageToSend;
    char commande[MAX_COMM];
    bool recInitied = false;
    int wrPipe;
    sread(0, commande, MAX_COMM);
    char action = commande[0];
    if (action == COMM_ADD_PROG) {
        messageToSend.code = ADD;
        char* path = commande+2; //points to first char of path
        putBackslash0(&path, 'c');
        strcpy(messageToSend.file.path, path);
        strcpy(messageToSend.file.nameFile.name, getFileNameFromPath(path));
        messageToSend.file.nameFile.size = strlen(messageToSend.file.nameFile.name)+1;
        askServerAddOrEditProgram(&messageToSend, socketServer);
    }else if (action == COMM_EXEC_PROG) {
        messageToSend.code = EXEC;
        int numProg = parseFirstInts(commande, 2, 3);
        askServerExecProgram(messageToSend, numProg, socketServer);
    }else if(action == COMM_EXECREC_PROG){
        if(!recInitied){
            wrPipe = initRecExecution(interval, socketServer);
            recInitied = true;
        }
        int numProg = parseFirstInts(commande, 2, 3);
        swrite(wrPipe, &numProg, sizeof(int));
    }else if (action == COMM_EDIT_PROG) {
        int numProg = parseFirstInts(commande, 2, 3);
        char numProgString[3];
        sprintf(numProgString, "%d", numProg);
        char* pntPathFile = commande+2; //points to first char of the number of the program
        pntPathFile += strlen(numProgString); //move to last char of the number of the program
        pntPathFile++; //move to the first char of the path by skipping the space
        messageToSend.numProg = numProg;
        putBackslash0(&pntPathFile, 'c');
        strcpy(messageToSend.file.path, pntPathFile);
        strcpy(messageToSend.file.nameFile.name, getFileNameFromPath(messageToSend.file.path));
        messageToSend.file.nameFile.size = strlen(messageToSend.file.nameFile.name)+1;
        askServerAddOrEditProgram(&messageToSend, socketServer);
    }else if (action == COMM_EXIT) {
        sclose(socketServer);
        printf("Au revoir!\n");
        exit(0);
    }else {
        printHelp();
    }
}



int main(int argc, char* argv[]) {
    serverIp = argv[1];
    serverPort = atoi(argv[2]);
    int interval = atoi(argv[3]);
    int sockFD;
    
    printf("Bienvenue dans le programme 2LCloudConsole\n\n");
    printHelp();
    while(true) {
        sockFD = initSocketClient(serverIp, serverPort);

        readCommandUser(sockFD, interval);
    }
}