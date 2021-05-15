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

void readServerResponse(int socketServer){
    ReturnMessage retM;
    sread(socketServer, &retM, sizeof(ReturnMessage));
    printf("numéro de programme: %d\n",retM.numProg);
    printf("état du programme: %d\n",retM.state);
    printf("temps d'exécution du programme: %d\n",retM.timeOfExecution);
    printf("code de retour: %d\n",retM.returnCode);
    printf("output:\n");
    char buff[OUTPUT_MAX];
    int nbCharLu = sread(socketServer, buff, OUTPUT_MAX);
    while(nbCharLu != 0) {
        nwrite(0, buff, nbCharLu);
        nbCharLu = sread(socketServer, buff, OUTPUT_MAX);
    }
}

void execProgram(StructMessage messageToReturn, int numProg, int socketServer){
    messageToReturn.code = EXEC;
    messageToReturn.numProg = numProg;
    swrite(socketServer, &messageToReturn, sizeof(messageToReturn));
    readServerResponse(socketServer);
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
                execProgram(messageToReturn, tab[i], *socketServer);
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

void askServerExecProgram(const StructMessage* messageToSend) {
    printf("%d\n", messageToSend->numProg);
}

void askServerAddProgram(const StructMessage* messageToSend, int socketServer) {
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
    readServerResponse(socketServer);

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

int initRecExecution(int interval, int sockfd/*, int* pipefd*/){
    int pipefd[2];
    //création du pipe
    spipe(pipefd);
    fork_and_run2(recurrentExec, &sockfd, &pipefd);
    //fermeture de la lecture sur le pipe
    sclose(pipefd[0]);
    fork_and_run2(timer, &interval, &pipefd);
    return pipefd[1];
}

StructMessage readCommandUser(int socketServer, int interval) {
    StructMessage messageToReturn;
    char commande[MAX_COMM];
    bool correctInput = false;
    bool recInitied = false;
    int wrPipe;
    printHelp();
    while (!correctInput) {
        sread(0, commande, MAX_COMM);
        char action = commande[0];
        if (action == COMM_ADD_PROG) {
            messageToReturn.code = ADD;
            char* path = commande+2; //points to first char of path
            char* pntLastChar = strrchr(commande, 'c');
            pntLastChar[1] = '\0';
            //path[strlen(path)-1] = '\0'; //remove \n
            strcpy(messageToReturn.file.path, path);
            char* pntFileName = strrchr(path, '/');
            pntFileName++; // move to after /
            strcpy(messageToReturn.file.nameFile ,pntFileName);
            swrite(socketServer, &messageToReturn, sizeof(messageToReturn));
            askServerAddProgram(&messageToReturn, socketServer);
            correctInput = true;
        }else if (action == COMM_EXEC_PROG) {
            messageToReturn.code = EXEC;
            int numProg = parseFirstInts(commande, 2, 3);
            correctInput = true;
            execProgram(messageToReturn, numProg, socketServer);
        }else if(action == COMM_EXECREC_PROG){
            if(!recInitied){
                wrPipe = initRecExecution(interval, socketServer);
                recInitied = true;
            }
             int numProg = parseFirstInts(commande, 2, 3);
             swrite(wrPipe, &numProg, sizeof(int));
        
        }else if (action == COMM_EXIT) {
            printf("Au revoir!\n");
            exit(0);
            //TODO: return null et appeler méthode qui quit le prog proprement.
        }else {
            printHelp();
        }
    }
    return messageToReturn;
}



int main(int argc, char* argv[]) {
    serverIp = argv[1];
    serverPort = atoi(argv[2]);
    int interval = atoi(argv[3]);
    int sockFD;
    
    sockFD = initSocketClient(serverIp, serverPort);

    printf("Bienvenue dans le programme 2LCloudConsole\n\n");
    readCommandUser(sockFD, interval);
}