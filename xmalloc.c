#include "xmalloc.h"
#include "semutex.h"

struct xheap_space_t xheap_space;  

int xheap_space_init(){
    int shm_id;
    if(xheap_space.top_chk){
        return 1;
    }

    // init shared memory
    shm_id = shmget(0, XHEAP_BASE_SIZE, IPC_CREAT | 0600);
    xheap_space.top_chk = shmat(shm_id, 0, 0);
    xheap_space.size = XHEAP_BASE_SIZE;
    xheap_space.unsorted_bin_list = NULL; 
    *((int*)(xheap_space.top_chk+8)) = XHEAP_BASE_SIZE; 

    // init semaphore
    xheap_space.sem_id = semget(0, 1, 0600|IPC_CREAT);
    init_mutex_sem(xheap_space.sem_id); 
    
    return 0;
}

void* xheap_malloc(size_t size){
    
    int remain_size;
    void* victim;
    void* tp_ck = xheap_space.top_chk; 
    int cks = S2CKSIZE(size+0x10);
    
    // gain mutex lock
    xheap_gain_lock(xheap_space.sem_id);

    // check size whether is invalid.
    if(cks >= XHEAP_BASE_SIZE)
        return 0;

    // TODO: check fast bin
    if(cks <= FAST_BIN_SIZE){
        
    }

    // TODO: check unsorted bin
    struct xheap_bin_ck_t* ub = (struct xheap_bin_ck_t*)(xheap_space.unsorted_bin_list);
    while(ub != NULL){

    }

    // split from top chk
    remain_size = CKSIZE(tp_ck);
    if (cks <= remain_size){
        victim = tp_ck;
        remain_size -= cks;
        *(int*)(tp_ck+8) = cks|INUSE_BIT_MASK;
        *(int*)(tp_ck+cks+8) = remain_size|INUSE_BIT_MASK;
        xheap_space.top_chk = tp_ck+cks;

        goto x_malloc_success;
    }

    victim = NULL;

x_malloc_success:
    xheap_release_lock(xheap_space.sem_id);
    return victim;
}

void* xheap_calloc(size_t size, size_t num){
    size_t total = size*num;
    void* buf = xheap_malloc(size*num);
    memset(buf, 0, total);
    return buf;
}

void xheap_free(void* victim){
    struct xheap_bin_ck_t* ck = (struct xheap_bin_ck_t*)(victim-0x10);
    int idx = CKS2IDX(ck->size);
   
    // unflag in-use bit
    ck->size &= (~INUSE_BIT_MASK);

    //set fd
    ck->fd = NULL;

    // gain lock and append to bin
    xheap_gain_lock(xheap_space.sem_id);
    struct xheap_bin_ck_t* bin = xheap_space.bin[idx];
    if(bin == NULL){
        xheap_space.bin[idx] = ck;
        return;
    }
    while(bin->fd != NULL){
        bin = bin->fd;
    }
    bin->fd = ck;
    
    xheap_release_lock(xheap_space.sem_id);
    return;
}

