#ifndef __MEM_MGR_H__
#define __MEM_MGR_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/******************************************************************************
 *
 * mem_mgr.h
 *
 * Author: Sudha Shunmugam 
 ****************************************************************************/

/********************************** includes *******************************/

/********************************** macros *********************************/

    
#define SEM_KEY         2000


/******************************* exported types ****************************/
typedef enum {
    FIRST_FIT=1,
    BEST_FIT=2,
    WORST_FIT=3
} alloc_algo_e_t; 
    
/************************* exported function prototypes ********************/
extern void * memory_malloc(int size);
extern int memory_free(void *ptr);
extern int mem_mgr_init(alloc_algo_e_t algo_type, int size);
extern int mem_mgr_shutdown();
extern void print_alloc_list();
extern void print_free_list();
extern int get_fragmented_mem_size();
extern void defragment_memory();
/***************************** exported globals ****************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __MEM_MGR_H__ */

/* end of mem_mgr.h */
