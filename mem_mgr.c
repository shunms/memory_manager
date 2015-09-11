#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/******************************************************************************
 *
 * mem_mgr.c
 *
 * Author: Sudha Shunmugam
 *         Computer Engineering Dept.
 *         UMASS Lowell
 *****************************************************************************/

#include <stdio.h>
#include <pthread.h>    // For Thread fuctions
#include <stdlib.h>     // For Malloc Function
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <signal.h>
#include "mem_mgr.h"

/* can also be set from Makefile */    
#if 0
#ifndef TOTAL_MEM_SIZE
#define TOTAL_MEM_SIZE  10240 /* 10 KB max for memory block */ 
#endif
#endif
int total_mem_size = 0;

extern void * request_allocator_to_malloc(int size);
extern int request_allocator_to_free(void *ptr);
    
typedef struct alloc_node_t {
    unsigned int thread_id;
    unsigned int size;
    unsigned char *addr;
    struct alloc_node_t *next;
} ALLOC_NODE;

typedef struct free_node_t {
    unsigned int size;
    unsigned char *addr;
    struct free_node_t *next;
} FREE_NODE;

unsigned char *mem_block_ptr=NULL;   /* ptr to the complete memory block */
ALLOC_NODE alloc_head;
FREE_NODE free_head;
alloc_algo_e_t alloc_algo = FIRST_FIT;

/************ Memory Manager Sub functions ***************/

void print_alloc_list()
{
    ALLOC_NODE * node_ptr = alloc_head.next;

    printf("Alloc_List\t: ");
    while(node_ptr != NULL) {
        printf("|%d - %x| --> ", node_ptr->size, node_ptr->addr);
        node_ptr = node_ptr->next;
    }
    printf("|NULL|\n");
    
}

void print_free_list()
{
    FREE_NODE * node_ptr = free_head.next;

    printf("Free_List\t: ");
    while(node_ptr != NULL) {
        printf("|%d - %x| --> ", node_ptr->size, node_ptr->addr);
        node_ptr = node_ptr->next;
    }
    printf("|NULL|\n");
}

int get_total_memory_size()
{
    return(total_mem_size);   
}

int get_fragmented_mem_size()
{
    FREE_NODE * node_ptr = free_head.next;
    int size = 0;
    
    while(node_ptr != NULL) {
        size += node_ptr->size;
        node_ptr = node_ptr->next;
    }
    size = total_mem_size - size;
    return(size);
}

void defragment_memory()
{
    FREE_NODE * free_node_ptr = free_head.next;
    FREE_NODE * temp = NULL;
    FREE_NODE * temp1 = NULL;
    ALLOC_NODE * alloc_node_ptr = alloc_head.next;
    
    /* No one has allocated - so regain the whole memory */
    if(alloc_node_ptr == NULL && free_node_ptr != NULL){
        free_node_ptr->size = total_mem_size;
        temp = free_node_ptr->next;
        free_node_ptr->next = NULL;
        while(temp != NULL) {
            temp1 = temp;
            temp = temp->next;
            free(temp1);            
        }
    } else {
        /* Real world scenario.Move the alloc in one side to gain free chunk */
    }
}

int allocate_mem_block() 
{
    mem_block_ptr = malloc(total_mem_size);
    if (mem_block_ptr == NULL) {
        printf("mem_mgr: failed to allocate space for memory block!\n");
        printf("mem_mgr: terminating program!\n");
        exit(2);
    }
}

init_alloc_head(ALLOC_NODE *node)
{
    node->thread_id = -1;
    node->size = 0;
    node->addr = NULL;
    node->next = NULL;
}

int init_free_head(FREE_NODE *node)
{
    node->size = total_mem_size;
    node->next = NULL;
}

int init_free_list ()
{
    FREE_NODE *first_node = malloc(sizeof(FREE_NODE));
    first_node->addr = mem_block_ptr;
    first_node->size = total_mem_size;
    first_node->next = NULL;
   
    /* Attach to free list head */
    free_head.next = first_node;
}

/* setup memory block globals */
int mem_mgr_init(alloc_algo_e_t algo_type, int size)
{
    alloc_algo = algo_type;
    total_mem_size = size;

    allocate_mem_block();
    init_alloc_head(&alloc_head);
    init_free_head(&free_head);
    init_free_list();
    print_alloc_list();
    print_free_list();
    printf("\n");
    return 0;
}

int mem_mgr_shutdown()
{
    if (mem_block_ptr != NULL) {
        free(mem_block_ptr);
        printf("mem_mgr_shutdown: Freed main memory block!\n");
    }
    else {
        printf("mem_mgr_shutdown: main memory block has already been freed!\n");
    }
        
    return 0;
}

ALLOC_NODE * create_new_alloc_node()
{
    return ((ALLOC_NODE *)malloc(sizeof(ALLOC_NODE)));
}

unsigned char * get_free_memory(unsigned int size)
{
    FREE_NODE *node_ptr = NULL;
    FREE_NODE *prev_node_ptr = NULL;
    unsigned char *allocated_region = NULL;

    int saved_size = 0;
    FREE_NODE *saved_node_ptr = NULL;
    FREE_NODE *saved_prev_node_ptr = NULL;

    node_ptr = free_head.next;
    prev_node_ptr = &free_head;
   
    /* Search for a free node with "size" memory based on the alloc_algo */
    switch (alloc_algo)
    {
        case FIRST_FIT:
            /* Find first free hole that fits the need */
            while (node_ptr != NULL) {
                if (node_ptr->size >= size) {
                    allocated_region = node_ptr->addr;
                    free_head.size -= size;
                    
                    if (node_ptr->size == size) {
                        prev_node_ptr->next = node_ptr->next;
                        free(node_ptr);
                    }
                    else {
                        node_ptr->addr += size;
                        node_ptr->size -= size;
                    }
                    break;
                }
                prev_node_ptr = node_ptr;
                node_ptr = node_ptr->next;
            }
            break;
        case BEST_FIT:
            /* Find free hole that best fits the need */
            saved_size = total_mem_size + 1;
            while (node_ptr != NULL) {
                if ((node_ptr->size >= size) && (node_ptr->size < saved_size)) {
                    saved_size = node_ptr->size;
                    saved_node_ptr = node_ptr;
                    saved_prev_node_ptr = prev_node_ptr;
                }
                prev_node_ptr = node_ptr;
                node_ptr = node_ptr->next;
            }
            if(saved_size != (total_mem_size + 1)) {
                allocated_region = saved_node_ptr->addr;
                free_head.size -= size;
                
                if (saved_node_ptr->size == size) {
                    saved_prev_node_ptr->next = saved_node_ptr->next;
                    free(saved_node_ptr);
                }
                else {
                    saved_node_ptr->addr += size;
                    saved_node_ptr->size -= size;
                }
            }
            break;
        case WORST_FIT:
            /* Find largest hole that fits the need */
            saved_size = 0;
            while (node_ptr != NULL) {
                if ((node_ptr->size >= size) && (node_ptr->size > saved_size)) {
                    saved_size = node_ptr->size;
                    saved_node_ptr = node_ptr;
                    saved_prev_node_ptr = prev_node_ptr;
                }
                prev_node_ptr = node_ptr;
                node_ptr = node_ptr->next;
            }
            if(saved_size != 0) {
                allocated_region = saved_node_ptr->addr;
                free_head.size -= size;
                
                if (saved_node_ptr->size == size) {
                    saved_prev_node_ptr->next = saved_node_ptr->next;
                    free(saved_node_ptr);
                }
                else {
                    saved_node_ptr->addr += size;
                    saved_node_ptr->size -= size;
                }
            }
            break;
        default:
            printf("mem_mgr: received invalid alloc_algo = %d\n", alloc_algo);
            return NULL;
    }
    
    return allocated_region;
}

int insert_into_alloc_list(ALLOC_NODE * new_node)
{
    ALLOC_NODE *prev_node_ptr = &alloc_head;
    ALLOC_NODE *node_ptr = alloc_head.next;
    
    while (node_ptr != NULL) {
        prev_node_ptr = node_ptr;
        node_ptr = node_ptr->next;
    }

    prev_node_ptr->next = new_node;
}

// allocated space from memory block 
void *mem_mgr_malloc(unsigned int size, int thread_id)
{
    unsigned int mem_size;
    ALLOC_NODE *alloc_node = NULL; 
    unsigned int align_byte = 4; /*  align to nearest 4 bytes */
    
    mem_size = size + ( align_byte - (size % align_byte ));  /* align to nearest align_byte boundary */

    /* create new node and get free memory */
    alloc_node = create_new_alloc_node();
    alloc_node->addr = get_free_memory(mem_size);
    if (alloc_node->addr == NULL) {
        printf("mem_mgr_malloc: has run out of memory. Cannot allocate %d bytes for thread id = %d\n", mem_size, thread_id);
        free(alloc_node);
        return NULL;
    }
    printf("mem_mgr_malloc: size = %d\tthread_id=%d\tptr=%x\ttotal_mem=%d\n", 
            mem_size, thread_id, alloc_node->addr, free_head.size);
    alloc_node->thread_id = thread_id;
    alloc_node->size = mem_size;
    alloc_node->next = NULL;

    insert_into_alloc_list(alloc_node);
#ifdef DEBUG
    print_alloc_list();
    print_free_list();
    printf("\n");
#endif
    return (void *)alloc_node->addr; 
}

FREE_NODE * create_new_free_node()
{
    return (FREE_NODE*)malloc(sizeof(FREE_NODE));
}

ALLOC_NODE * get_alloc_node (void *ptr, int thread_id) 
{
    ALLOC_NODE *node_ptr = alloc_head.next;
    
    while (node_ptr != NULL) {
        if ((node_ptr->addr == (unsigned char *)ptr) && (node_ptr->thread_id == thread_id))
            break;
        node_ptr = node_ptr->next;
    }

    return node_ptr;
}

int insert_into_free_list(FREE_NODE * new_node)
{
    FREE_NODE *prev_node_ptr = &free_head;
    FREE_NODE *node_ptr = free_head.next;

    if(node_ptr == NULL) {
        prev_node_ptr->next = new_node;
        new_node->next = node_ptr;
        free_head.size += new_node->size;
        return 0;
    }
    while (node_ptr != NULL) {
        if (node_ptr->addr > new_node->addr) {
            /* Coalesce */
            if((prev_node_ptr->addr+ prev_node_ptr->size) == new_node->addr){
                prev_node_ptr->size += new_node->size;
                free_head.size += new_node->size;
                free(new_node);
                if((prev_node_ptr->addr + prev_node_ptr->size) == node_ptr->addr) {
                    prev_node_ptr->size += node_ptr->size;
                    prev_node_ptr->next = node_ptr->next;
                    free(node_ptr);
                }
                return(1);
            } else if((new_node->addr + new_node->size) == node_ptr->addr) {
                node_ptr->addr = new_node->addr;
                node_ptr->size += new_node->size;
                free_head.size += new_node->size;
                free(new_node);
                return(2);
            }
            prev_node_ptr->next = new_node;
            new_node->next = node_ptr;
            free_head.size += new_node->size;
            return 0;
        }
        prev_node_ptr = node_ptr;
        node_ptr = node_ptr->next;
    }

    return -1;
}

int delete_from_alloc_list(ALLOC_NODE * node_to_free)
{
    ALLOC_NODE *prev_node_ptr = &alloc_head;
    ALLOC_NODE *node_ptr = alloc_head.next;
    
    while (node_ptr != NULL) {
        if(node_ptr == node_to_free) {
            prev_node_ptr->next = node_ptr->next;
            free(node_ptr);
            return(0);
        }
        prev_node_ptr = node_ptr;
        node_ptr = node_ptr->next;
    }

    return(-1);
}

// free space back to memory block 
void mem_mgr_free(void *ptr, int thread_id)
{
    ALLOC_NODE *node_to_free;
    FREE_NODE *new_node;
   
    node_to_free = get_alloc_node(ptr, thread_id);
    if (node_to_free != NULL) {
        new_node = create_new_free_node();
        
        new_node->size = node_to_free->size;
        new_node->addr = node_to_free->addr;
        new_node->next = NULL;
        
        printf("mem_mgr_free: size = %d\tthread_id=%d\tptr=%x\t", 
                new_node->size, thread_id, new_node->addr);
        insert_into_free_list(new_node);
        delete_from_alloc_list(node_to_free);
        printf("total_mem = %d\n", free_head.size);
#ifdef DEBUG
        print_alloc_list();
        print_free_list();
        printf("\n");
#endif
    }
}

/* called by user threads to request memory */
void *memory_malloc(int size)
{
    return request_allocator_to_malloc(size);
}

/* called by user threads to free memory */
int memory_free(void *ptr)
{
    return request_allocator_to_free(ptr);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
/* end of mem_mgr.c */
