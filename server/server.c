#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../utils_v10.h"

#define MAX_CLIENTS 50

void addProgram(File fileToCreate, int socket) {
    int fdNewFile = sopen("programs/test.c", O_CREAT | O_TRUNC | O_WRONLY, 0777);
    printf("Le client veut ajouter un programme\n");
    printf("Nom du programme: %s\n", fileToCreate.nameFile);
    char fileBlock[BLOCK_FILE_MAX];
    printf("En attente du contenu du fichier à ajouter ...\n");
    int nbCharLu = sread(socket, fileBlock, BLOCK_FILE_MAX);
    int i = 1;
    while(nbCharLu != 0) {
        swrite(fdNewFile, fileBlock, nbCharLu);
        nbCharLu = sread(socket, fileBlock, BLOCK_FILE_MAX);
        i++;
    }
    printf("Tout le fichier a été recu !\n");
    
}

void execProgram(int numProg) {
    printf("Le client veut exec le prog num: %d\n", numProg);

}

void execution(void* arg1, void* arg2, void* socket){
    int* clientSocketFD = socket;
    dup2(*clientSocketFD,1);
    char* a1 = arg1;
    char* a2 = arg2;
    sexecl(a1, a2,NULL);
}

void changeInformationAfterExec(int numProg, long timeSpent){
    // GET SEMAPHORE
    int sem_id = sem_get(SEM_KEY, 1);

    // GET SHARED MEMORY
    int shm_id = sshmget(SHM_KEY, sizeof(files), 0);
    files* f = sshmat(shm_id);

    sem_down0(sem_id);
    f->tab[numProg].numberOfExecutions++;
    f->tab[numProg].totalTimeExecution += timeSpent;
    sshmdt(f);
    sem_up0(sem_id); 

}

MessageAfterExecution execProgram(int numProg, void* socket) {
    MessageAfterExecution mae;
    printf("Le client veut exec le prog num: %d\n", numProg);
    struct timeval start, end;
    char arg1[25];
    char arg2[25];
    sprintf(arg1, "./server/programs/%d",numProg);
    sprintf(arg2, "server/programs/%d",numProg);

    gettimeofday(&start, NULL);

    int childId = fork_and_run3(execution, &arg1, &arg2, socket);
    swaitpid(childId, &mae.returnCode, 0);

    gettimeofday(&end, NULL);
    long timeSpentSeconds = (end.tv_sec - start.tv_sec);
    long timeSpentMicro = (end.tv_usec - start.tv_usec);
    int timeSpent = timeSpentSeconds*1000000 + timeSpentMicro;

    changeInformationAfterExec(numProg, timeSpent);

    return mae;
}

//thread lié à un client
void clientProcess(void* socket) {
    int* clientSocketFD = socket;
    StructMessage message;
    sread(*clientSocketFD, &message, sizeof(message));
    if (message.code == ADD) {
        addProgram(message.file, *clientSocketFD);
    }else if (message.code == EXEC) {
       //MessageAfterExecution mae;
        //mae = execProgram(message.numProg, socket);
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