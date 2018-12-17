#ifndef __X_MALLOC_H_
#define __X_MALLOC_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// each bin size
#define FAST_BIN_SIZE       0x80
#define SMALL_BIN_SIZE      0x400
#define LARGE_BIN_SIZE      0x1000
#define XHEAP_BASE_SIZE     0x10000

// Mask
#define INUSE_BIT_MASK      0x1

// chunk macro
#define S2CKSIZE(size)          ( (size)&0x8 ? ((size)&(~(0xf)))+0x10 :  (size)&(~(0xf)) )
#define CKSIZE(chunk)           ( *(uint32_t*)((chunk)+8) )
#define CKS2IDX(size)           ( ((size)>>4) - 2 )

struct shm_info_list_t{
    int shm_id;
    struct shm_info_list_t* next;
};

struct xheap_space_t{
    void*   top_chk;
    void*   unsorted_bin_list;
    void*   bin[0x100-1];
    int     sem_id;
    struct shm_info_list_t* shm_info_list;
    size_t  size;    
};

struct xheap_bin_ck_t{
    uint64_t flag;
    uint32_t useless;
    uint32_t size;
    struct xheap_bin_ck_t* fd;
    struct xheap_bin_ck_t* bk;

};

int xheap_init();

void* xheap_malloc(size_t size);
void* xheap_calloc(size_t size, size_t num);
void xheap_free(void* victim);
#endif
