#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

#include "../utils_v10.h"

#define COMM_ADD_PROG '+'
#define COMM_EDIT_PROG '.'
#define COMM_EXEC_PROG '@'
#define COMM_EXIT 'q'
#define COMM_HELP 'h'

#define MAX_COMM 255

void printHelp() {
    printf("Commandes disponibles:\n");
    printf("\t- %c: <chemin d'un fichier C>: Permet d'ajouter le programme situé au chemin spécifié\n", COMM_ADD_PROG);
    printf("\t- %c: num <chemin d'un fichier C>: Permet de remplacer le programme associé portant le numéro 'num'\n", COMM_EDIT_PROG);
    printf("\t- %c: num: Permet de demander au serveur d'exécuter le programme ayant le numéro 'num'\n", COMM_EXEC_PROG);
    printf("\t- %c: Permet d'afficher l'aide\n", COMM_HELP);
    printf("\t- %c: Quitte le programme\n", COMM_EXIT);
}

void askServerExecProgram(const StructMessage* messageToSend) {
    printf("%d\n", messageToSend->numProg);
}

void askServerAddProgram(const StructMessage* messageToSend, int socketServer) {
    //envoie fichier vers server
    int fd = sopen(messageToSend->file.path, O_RDONLY, 0600);
    char buff[BLOCK_FILE_MAX];
    int nbCharLu = sread(fd, buff, BLOCK_FILE_MAX);
    while(nbCharLu != 0) {
        nwrite(socketServer, buff, nbCharLu);
        nbCharLu = sread(fd, buff, BLOCK_FILE_MAX);
    }
    int ret = shutdown(socketServer, SHUT_WR);
    checkNeg(ret, "ERROR shutdown\n");

    //attente réponse server
    // StructMessage responseServer;
    // nbCharLu = sread(socketServer, &responseServer, sizeof(responseServer));

}

void readServerResponse(int socketServer){
    ReturnMessage retM;
    sread(socketServer, &retM, sizeof(ReturnMessage));
    printf("%d\n",retM.numProg);
    printf("%d\n",retM.state);
    printf("%d\n",retM.timeOfExecution);
    printf("%d\n",retM.returnCode);
    printf("output:\n");
    char buff[OUTPUT_MAX];
    int nbCharLu = sread(socketServer, buff, OUTPUT_MAX);
    while(nbCharLu != 0) {
        nwrite(0, buff, nbCharLu);
        nbCharLu = sread(socketServer, buff, OUTPUT_MAX);
    }
}

StructMessage readCommandUser(int socketServer) {
    StructMessage messageToReturn;
    char commande[MAX_COMM];
    bool correctInput = false;
    printHelp();
    while (!correctInput) {
        sread(0, commande, MAX_COMM);
        char action = commande[0];
        if (action == COMM_ADD_PROG) {
            messageToReturn.code = ADD;
            char* path = commande+2; //points to first char of path
            path[strlen(path)-1] = '\0'; //remove \n
            strcpy(messageToReturn.file.path, path);
            char* pntFileName = strrchr(path, '/');
            pntFileName++; // move to after /
            strcpy(messageToReturn.file.nameFile ,pntFileName);
            swrite(socketServer, &messageToReturn, sizeof(messageToReturn));
            askServerAddProgram(&messageToReturn, socketServer);
            correctInput = true;
        }else if (action == COMM_EXEC_PROG) {
            messageToReturn.code = EXEC;
            int ret = parseFirstInts(commande, 2, 3);
            messageToReturn.numProg = ret;
            //askServerExecProgram(&messageToReturn);
            swrite(socketServer, &messageToReturn, sizeof(messageToReturn));
            correctInput = true;
            readServerResponse(socketServer);
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
    char* serverIp = argv[1];
    int serverPort = atoi(argv[2]);
    int sockFD;

    sockFD = initSocketClient(serverIp, serverPort);
    
    printf("Bienvenue dans le programme 2LCloudConsole\n\n");
    /*StructMessage message = */readCommandUser(sockFD);


}