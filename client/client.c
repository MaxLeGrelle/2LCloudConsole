

#include "../utils_v10.h"

// #define COMM_ADD_PROG '+\n'
// #define COMM_EDIT_PROG '.\n'
// #define COMM_EXEC_PROG '@\n'
// #define COMM_EXIT 'q\n'
// #define COMM_HELP 'h\n'

const static char COMM_ADD_PROG = '+';
//const static char* COMM_EDIT_PROG = ".\n";
const static char COMM_EXEC_PROG = '@';
const static char COMM_EXIT = 'q';

#define MAX_COMM 255

void printHelp() {
    printf("Commandes disponibles:\n");
    printf("\t- + <chemin d'un fichier C>: Permet d'ajouter le programme situé au chemin spécifié\n");
    printf("\t- . num <chemin d'un fichier C>: Permet de remplacer le programme associé portant le numéro 'num'\n");
    printf("\t- @ num: Permet de demander au serveur d'exécuter le programme ayant le numéro 'num'\n");
    printf("\t- h: Permet d'afficher l'aide\n");
    printf("\t- q: Quitte le programme\n");
}


bool isNumber(char c) {
    return c >= '0' && c <= '9';
}

int convertToInt(char c) {
    return c - '0';
}

int parseFirstInts(char* s, int nbIntToParse) {
    if (strlen(s) == 1 && isNumber(s[0])) {
        return convertToInt(s[0]);
    }
    char* stringOfNumbers = (char*)malloc(nbIntToParse*sizeof(char));
    if (stringOfNumbers == NULL) return -1;
    for (int i = 0; i < strlen(s)+1; i++) {
        if (isNumber(s[i])) {
            strncat(stringOfNumbers, &s[i], 1);
        }
        if (strlen(stringOfNumbers) == nbIntToParse) break;
    }
    int nbParsed = atoi(stringOfNumbers);
    free(stringOfNumbers);
    return nbParsed;
}

void askServerExecProgram(int number) {
    printf("%d\n", number);
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
            messageToReturn.numProg = parseFirstInts(commande, 3);
            askServerExecProgram(messageToReturn.numProg);
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

    swrite(sockFD, &message, sizeof(message));
}