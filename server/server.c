#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../utils_v10.h"

#define MAX_CLIENTS 50

void compile(void* path) {

}

void addProgram(File fileToCreate, int socket) {
    printf("Le client veut ajouter un programme\n");
    printf("Nom du programme: %s\n", fileToCreate.nameFile);
    char path[255];
    char* dir = "programs/";
    strcat(path, dir);
    strcat(path, fileToCreate.nameFile);
    //lecture du fichier à ajouter venant du client.
    int fdNewFile = sopen(path, O_CREAT | O_WRONLY, 0777);
    char fileBlock[BLOCK_FILE_MAX];
    printf("En attente du contenu du fichier à ajouter ...\n");
    int nbCharLu = sread(socket, fileBlock, BLOCK_FILE_MAX);
    while(nbCharLu != 0) {
        nwrite(fdNewFile, fileBlock, nbCharLu);
        nbCharLu = sread(socket, fileBlock, BLOCK_FILE_MAX);
    }
    //Compilation du fichier.
    // fork_and_run1(compile, path);
    printf("Tout le fichier a été recu !\n");
    
}

void execProgram(int numProg) {
    printf("Le client veut exec le prog num: %d\n", numProg);

}

//thread lié à un client
void clientProcess(void* socket) {
    int* clientSocketFD = socket;
    StructMessage message;
    sread(*clientSocketFD, &message, sizeof(message));
    if (message.code == ADD) {
        addProgram(message.file, *clientSocketFD);
    }else if (message.code == EXEC) {
        execProgram(message.numProg);
    }
}

int main(int argc, char* argv[]) {
    int port = atoi(argv[1]);
    int clientSocketFD;

    //init server's socket
    int servSockFD = initSocketServer(port);
    printf("Le serveur est à l'écoute sur le port %d\n", port);

    //listening if a client want to connect or communicate with the server.
    while(true) {
        clientSocketFD = saccept(servSockFD);
        printf("Un client s'est connecté ! : %d\n", clientSocketFD);
        
        //création d'un enfant par client qui se connecte.
        fork_and_run1(clientProcess, &clientSocketFD);
    }
}