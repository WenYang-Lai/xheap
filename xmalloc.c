#include "xmalloc.h"
#include "semutex.h"

struct xheap_space_t* xheap_space = NULL;  

int xheap_space_init(){
    int shm_id;
    void* addr;
    int size;

    if(xheap_space){
        return 1;
    }

    // init shared memory
    shm_id = shmget(0, XHEAP_BASE_SIZE, IPC_CREAT | 0600);
    addr = shmat(shm_id, 0, 0);
    size = XHEAP_BASE_SIZE - sizeof(struct xheap_space_t);
    
    xheap_space = addr;
    xheap_space->top_chk = addr + sizeof(struct xheap_space_t);
    xheap_space->size = size;
    xheap_space->unsorted_bin_list = NULL ; 
    
    memset(xheap_space->bin , 0, sizeof(struct xheap_bin_ck_t*)*0xff);
    // set top ck size
    *((int*)(xheap_space->top_chk+8)) = size|INUSE_BIT_MASK; 

    // init semaphore
    xheap_space->sem_id = semget(0, 1, 0600|IPC_CREAT);
    init_mutex_sem(xheap_space->sem_id); 
    
    return 0;
}

void* xheap_malloc(size_t size){
    
    int remain_size;
    int idx;
    int ubs;
    void* victim = NULL;
    void* tp_ck = xheap_space->top_chk; 
    struct xheap_bin_ck_t* next_ck;
    int cks = MAX(S2CKSIZE(size+0x10), MINCKSIZE);
    
    // gain mutex lock
    xheap_gain_lock(xheap_space->sem_id);

    // check size whether is invalid.
    if(cks >= XHEAP_BASE_SIZE)
        goto x_malloc_fail;

    idx = CKS2IDX(cks);

    // TODO: check fast bin
    if(cks <= FAST_BIN_SIZE){
        victim = xheap_find_bin(idx);
        if(victim)  goto x_malloc_success;
    }

    // TODO: check unsorted bin
    struct xheap_bin_ck_t* ub = (struct xheap_bin_ck_t*)(xheap_space->unsorted_bin_list);
    while(ub != NULL){
        ubs = CKS2SIZE(ub->size);
        if (cks == ubs){
            next_ck = (struct xheap_bin_ck_t*)(((void*)ub)+CKS2SIZE(ub->size));
            next_ck->size |= INUSE_BIT_MASK;
            
        }
        else{
            
        }
    }

    victim = xheap_find_bin(idx);
    if (victim)
        goto x_malloc_success;
    
    // split from top chk
    remain_size = CKSIZE(tp_ck);
    printf("pid: %d, request: %x, remain: %x \n", getpid(), cks, remain_size);
    if (cks < remain_size){
        victim = tp_ck+0x10;
        remain_size -= cks;
        *(int*)(tp_ck+8) = cks|INUSE_BIT_MASK;
        *(int*)(tp_ck+cks+8) = remain_size|INUSE_BIT_MASK;
        xheap_space->top_chk = tp_ck+cks;

        goto x_malloc_success;
    }

x_malloc_fail:
    victim = NULL;

x_malloc_success:
    xheap_release_lock(xheap_space->sem_id);
    return victim;
}

void* xheap_calloc(size_t size, size_t num){
    size_t total = size*num;
    void* buf = xheap_malloc(total);
    if (buf){
        memset(buf, 0, total);
    }
    return buf;

}

void* xheap_find_bin(int idx){
    struct xheap_bin_ck_t* ck = xheap_space->bin[idx];
    if(ck){
        xheap_space->bin[idx] = ck->fd;
        return ((void*)ck)+0x10;
    }
    return NULL;
}

void xheap_insert_ck2bin(struct xheap_bin_ck_t* ck){
    int idx = CKS2IDX(ck->size);
    struct xheap_bin_ck_t* bin_ck = xheap_space->bin[idx];

    if(bin_ck){
        ck->fd = bin_ck;
    }   
    xheap_space->bin[idx] = ck;
    return;
}

void xheap_free(void* victim){
    struct xheap_bin_ck_t *ck, *next_ck;
    
    ck = (struct xheap_bin_ck_t*)(victim-0x10);
    next_ck = (struct xheap_bin_ck_t*)(victim+ck->size-0x10);
    
    // gain lock and append to bin
    xheap_gain_lock(xheap_space->sem_id);
    
    fprintf(stderr, "free %x size chunk\n", ck->size);
    if (next_ck == xheap_space->top_chk){
        ck->size += CKSIZE(xheap_space->top_chk);
        xheap_space->top_chk = (void*)ck;
        goto x_free_success;
    }

    //set fd
    ck->fd = NULL;

    xheap_insert_ck2bin(ck);

x_free_success:
    xheap_release_lock(xheap_space->sem_id);
    return;
}
