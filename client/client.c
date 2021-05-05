#include "../utils_v10.h"

#define COMM_ADD_PROG '+\n'
#define COMM_EDIT_PROG '.\n'
#define COMM_EXEC_PROG '@\n'
#define COMM_EXIT 'q\n'
#define COMM_HELP 'h\n'

#define MAX_COMM 2

void printHelp() {
    printf("Commandes disponibles:\n");
    printf("\t+ <chemin d'un fichier C>: Permet d'ajouter le programme situé au chemin spécifié\n");
    printf("\t. num <chemin d'un fichier C>: Permet de remplacer le programme associé portant le numéro 'num'\n");
    printf("\t@ num: Permet de demander au serveur d'exécuter le programme ayant le numéro 'num'\n");
    printf("\th: Permet d'afficher l'aide\n");
    printf("\tq: Quitte le programme\n");
}

int main(int argc, char* argv[]) {
    char* serverIp = argv[1];
    int serverPort = atoi(argv[2]);
    int sockFD;
    StructMessage message;

    sockFD = initSocketClient(serverIp, serverPort);
    
    printf("Bienvenue dans le programme 2LCloudConsole\n");
    printHelp();
    sread(0, &message, sizeof(message));

    swrite(sockFD, &message, sizeof(message));
}