#ifndef __X_HEAP_MUTEX_
#define __X_HEAP_MUTEX_

#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>

#define xheap_gain_lock(sem_id) \
    if(!sem_wait((sem_id))){ \
        perror("semaphore error!"); \
        exit(1); \
    }

#define xheap_release_lock(sem_id) \
    if(!sem_signal((sem_id))){ \
        perror("semaphore error!"); \
        exit(1); \
    }


union semun{
    int     val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

struct sembuf sem_op_v = {0, 1, SEM_UNDO};
struct sembuf sem_op_p = {0, -1, SEM_UNDO};


int init_mutex_sem(int sem_id){
    union semun sem_union;
    sem_union.val = 1;
    if(semctl(sem_id, 0, SETVAL, sem_union) == -1){
        perror("Init semaphore failed");
        return -1;
    }
    return 0;
}

int sem_wait(int sem_id){
    if(semop(sem_id, &sem_op_p, 1) == -1){
        return 0;
    }
    return 1;
}
int sem_signal(int sem_id){
    if(semop(sem_id, &sem_op_v, 1) == -1){
        return 0;
    }
    return 1;
}
#endif
