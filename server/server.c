#include <sys/time.h>
#include "../utils_v10.h"
#include "./filesInfo.h"

#define SHM_KEY 493 //shared memory key
#define SEM_KEY 925 //semaphore key
#define MAX_CLIENTS 50

void addProgram(File fileToCreate) {
    printf("Le client veut ajouter un programme\n");
    printf("Nom du programme: %s\nNombre de bytes: %d", fileToCreate.nameFile, fileToCreate.size);
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
        addProgram(message.file);
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