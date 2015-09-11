/*--------------- user.c ---------------*/
/*
BY:     Sudha Shunmugam
        Computer Engineering Dept.
        UMASS Lowell
*/

/*
PURPOSE
Assignment 4  - Operating Systems Course (16.573)

CHANGES
11-10-2006 -  Created.
12-02-2006 -  Added common things in common.h 

*/
/************* Include files **************/
#include <stdio.h>
#include <pthread.h>    // For Thread fuctions
#include <stdlib.h>     // For Malloc Function
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <signal.h>
#include "common.h"
#include "mem_mgr.h" 

/*********** Constant Declaration **********/


/*********** Function Prototypes **********/
extern void * memory_malloc(int size);
extern int memory_free(void *ptr);
extern int get_total_memory_size();

extern int semid, g_allocated, g_freed;

/********** User thread program *************/
void *user_thread(void * arg)
{
    char *ptr;
    int i;
    unsigned short seed[3];
	struct timeval 	randtime;
    unsigned int mem_size;
    struct drand48_data buffer;


    /* Get the current time and use that as seed */
	gettimeofday ( &randtime, ( struct timezone * ) 0 );
	seed[0] = (ushort)randtime.tv_usec;
	seed[1] = (ushort) (randtime.tv_usec >> 16 );
	seed[2] = (ushort) (pthread_self());
#if 0
	seed[0] = 10;
	seed[1] = 100;
	seed[2] = 1000;
#endif
    
    seed48_r(seed, &buffer);
        
    for(i = 0; i < 10; i++) {
        nrand48_r(seed, &buffer, (long int *)&mem_size);
        mem_size = mem_size % get_total_memory_size(); 
        //mem_size = 100;
        printf("Waiting for semaphore\n");
        
        sem_wait(&semid);
        ptr = (char *)memory_malloc(mem_size);
        if(ptr != NULL){
            g_allocated++;
        }
        sem_post(&semid);
        printf("Allocated and posted sem\n");

        usleep(1);

        sem_wait(&semid);
        if(ptr != NULL){
            memory_free(ptr);
            g_freed++;
        }
        printf("Freed sem\n");
        sem_post(&semid);
    }
}


