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
    
    printf("%s\n", titleOutput);
    char buff[OUTPUT_MAX];
    int nbCharLu = sread(socketServer, buff, OUTPUT_MAX);
    while(nbCharLu != 0) {
        nwrite(0, buff, nbCharLu);
        nbCharLu = sread(socketServer, buff, OUTPUT_MAX);
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

void putBackslash0(char** s, char c) {
    char* pntLastChar = strrchr(*s, c); //points to last char
    pntLastChar[1] = '\0'; //remove \n
}

char* getFileNameFromPath(char* path) {
    char* pntFileName = strrchr(path, '/'); //points to /name_of_prog
    pntFileName++; // move to after /
    return pntFileName;
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