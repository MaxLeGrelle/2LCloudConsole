#include "../utils_v10.h"

#define MAX_CLIENTS 50

void addProgram(File fileToCreate) {
    printf("Le client veut ajouter un programme\n");
    printf("Nom du programme: %s\nNombre de bytes: %d", fileToCreate.nameFile, fileToCreate.size);
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
        addProgram(message.file);
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