#include "../utils_v10.h"
#include "./filesInfo.h"

#define SHM_KEY 493 //shared memory key
#define SEM_KEY 925 //semaphore key
#define PERM 0666
#define CREATE 1
#define DELETE 2
#define RESERVE 3 

void deleteMemory(int shm_id, int sem_id);
void reserveMemory(int shm_id, int sem_id, int duree);

int main(int argc, char *argv[]){ 

    int shm_id = 0;
    int sem_id = 0;

    //type of function
    int type = atoi(argv[1]);

    if(type == CREATE){
        printf("création de la mémoire partagée \n");
        //create semaphore: 1 acces
        sem_id =sem_create(SEM_KEY, 1, PERM, 1);

        //create shared memory
        shm_id = sshmget(SHM_KEY,sizeof(files), IPC_CREAT | PERM);

        files f;
        f.size = 0;
        files* varMemo = sshmat(shm_id);
        
        *varMemo = f;
    }

    else if(type == DELETE){
        deleteMemory(shm_id, sem_id);
    }
    else if(type == RESERVE){
        reserveMemory(shm_id, sem_id, atoi(argv[2]));
    }
}

void deleteMemory(int shm_id, int sem_id){
    printf("destruction de la mémoire partagée \n");
    sshmdelete(shm_id);
    sem_delete(sem_id);
}

void reserveMemory(int shm_id, int sem_id, int duree){
    printf("down pour %d secondes \n", duree);
    sem_down0(sem_id);
    sleep(duree);
    sem_up0(sem_id);
    printf("up\n");
}