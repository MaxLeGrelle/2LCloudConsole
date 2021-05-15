#include <stdbool.h>

#include "../utils_v10.h"
#include "serverUtil.h"

int main(int argc, char *argv[]){

    int fileNumber = atoi(argv[1]);
     
    // GET SEMAPHORE
    int sem_id = sem_get(SEM_KEY, 1);

    // GET SHARED MEMORY
    int shm_id = sshmget(SHM_KEY, sizeof(files), 0);
    files* f = sshmat(shm_id);

    sem_down0(sem_id);
    
    if( fileNumber >= f->size){//no stat to show
        printf("numero de fichier inexistant");
        sem_up0(sem_id);
        sshmdt(f); 
        _exit(1);
    }
    else{ //show stat about
        printf("%d\n",f->tab[fileNumber].number);
        printf("%s\n",f->tab[fileNumber].name);
        printf("%s\n", f->tab[fileNumber].compile==0 ? "true" : "false");
        printf("%d\n",f->tab[fileNumber].numberOfExecutions);
        printf("%d\n",f->tab[fileNumber].totalTimeExecution);
        sshmdt(f);
        sem_up0(sem_id); 
    }
    _exit(0);

}