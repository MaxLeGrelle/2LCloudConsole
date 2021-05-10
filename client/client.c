

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

void askServerExecProgram(const StructMessage* messageToSend, int socketServer) {
    printf("%d\n", messageToSend->numProg);
    swrite(socketServer, messageToSend, sizeof(messageToSend));
}

StructMessage readCommandUser() {
    StructMessage messageToReturn;
    char commande[MAX_COMM];
    bool correctInput = false;
    printHelp();
    while (!correctInput) {
        sread(0, commande, MAX_COMM);
        char action = commande[0];
        if (action == COMM_ADD_PROG) {
            messageToReturn.code = ADD;
            correctInput = true;
        }else if (action == COMM_EXEC_PROG) {
            messageToReturn.code = EXEC;
            int ret = parseFirstInts(commande, 2, 3);
            if (ret == -1) exit(1);
            messageToReturn.numProg = ret;
            correctInput = true;
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
    StructMessage message = readCommandUser();
    askServerExecProgram(&message, sockFD);

}