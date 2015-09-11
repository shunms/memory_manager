/*--------------- main.c ---------------*/
/*
BY:     Sudha Shunmugam
        Computer Engineering Dept.
        UMASS Lowell
*/

/*
PURPOSE
Assignment 4  - Operating Systems Course (16.573)

CHANGES
11-25-2006 -  Created.
11-25-2006 -   

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
int num_threads;
pthread_t *user_thread_id, sig_wait_id, allocator_id;
int *pdata, semid;
long allocator_channel;
int g_allocated = 0;
int g_freed = 0;
int alloc_algorithm = 0;

/*********** Function Prototypes **********/
void kill_child();
void *sig_waiter(void *arg);
void print_report();

extern void *user_thread(void *arg);
extern void *allocator_thread(void *arg);
extern void create_allocator_queue();
extern void delete_allocator_queue();
extern int get_total_memory_size();

/*********** Main program **********/
int main()
{
    int i,nsigs;
    char temp[20];
    int total_mem_size = 0;
    sigset_t  all_signals;
    int sigs[] = { SIGBUS, SIGSEGV, SIGFPE };

    pthread_attr_t thread_attr;
    struct sched_param  sched_struct;
    struct sigaction new_sig_action; 
        
    while(1) {
        system("clear");

        printf("Memory Manager. Press Control+C to exit anytime\n");
        
        /* Get the Number of threads from User */
        printf("\nEnter the Number of Threads[1 - 1000], 0 - Exit\n");
        fgets(temp, 19, stdin); /* fgets is the best way to read an input */

        num_threads = atoi(temp);
        
        /* Validate the User Input */
        if(!(isdigit(temp[0])) || num_threads < 0 || num_threads > 1000){
            printf("Invalid Value for Number of Threads.\
                    \nPlease press <Enter> to Continue\n");
            getchar();
            continue;
        }

        if(num_threads == 0) {
            printf("Thank You. Bye\n");
            exit(0);
        } else {
            printf("\nMemory Manager Configuration\n");
            while(1) {
                printf("Enter the Total Memory Size: Minimum 256 Bytes Maximum 16K\n");
                fgets(temp, 19, stdin); 
                total_mem_size = atoi(temp);
                if(!(isdigit(temp[0])) || total_mem_size < 0){
                    printf("Invalid Input for memory size \
                            \n Please press Enter to continue\n");
                    getchar();
                    continue;
                }
                break;
            }
            for(i = 256; i < 16384 ; i = i << 1) {
                if(i >= total_mem_size){
                    printf("Rounded the value to the nearest power of 2. ");
                    break;
                }
            }
            total_mem_size = i;
            printf("Size accepted in system = %d\n", total_mem_size);

            while(1) {
                printf("\n Enter the type of algorithm\n");
                printf("1. First Fit\n");
                printf("2. Best Fit\n");
                printf("3. Worst Fit\n");
                fgets(temp, 19, stdin); 
                alloc_algorithm = atoi(temp);
                if(!(isdigit(temp[0])) ||alloc_algorithm > 3 || alloc_algorithm <= 0){
                    printf("Invalid Input for Algorithm Type \
                            \nPlease press Enter to continue\n");
                    getchar();
                    continue;
                }
                break;
            }
            break;
        }
    }
    
    allocator_channel = num_threads+1;

    create_allocator_queue();

    mem_mgr_init((alloc_algo_e_t)alloc_algorithm, total_mem_size);

    /* Create the two semaphores - 1st one should be the number of consumers*/
    if((semid = semget((key_t)SEM_KEY, 1, IPC_CREAT|0666)) == -1) {
        printf("!!!Error: Can not create semaphore1\n");
        exit(1);
    }

    /* Turning all bits on ie, setting to 1 */
    sigfillset (&all_signals );
    nsigs = sizeof ( sigs ) / sizeof ( int );
        
    /* Now setting all those bits off for each signal
     * that we are interested in
     */
    for ( i = 0; i < nsigs; i++ )
        sigdelset ( &all_signals, sigs [i] );
    sigprocmask ( SIG_BLOCK, &all_signals, NULL );

    /* Create a thread to wait for the control+c to quit the program */
    if ( pthread_create(&sig_wait_id, NULL, sig_waiter, NULL) != 0 ){
        printf("pthread_create failed for sig wait thread ");
        exit(3);
    }
    
    if ( pthread_create(&allocator_id, NULL, allocator_thread, NULL) != 0 ){
        printf("pthread_create failed for allocator thread");
        exit(3);
    }

    pthread_attr_init ( &thread_attr );
    pthread_attr_setinheritsched ( &thread_attr,
            PTHREAD_INHERIT_SCHED );

    /* Allocate thread related data structures */
    user_thread_id = (pthread_t *)malloc((num_threads + 1) * sizeof(pthread_t));
    pdata = (int *)malloc((num_threads + 1) * sizeof(int));

    /* Create user threads */
    for(i = 0; i < num_threads; i++) {
        pdata[i] = i;
        if(pthread_create(&user_thread_id[i], &thread_attr, user_thread, &pdata[i]) != 0)
            printf("Pthread create failed for user thread %d\n", i);
    }

    for(i = 0; i < num_threads; i++){
        pthread_join(user_thread_id[i], NULL);
    }

    /* Kill the sig_waiter thread */
    pthread_kill(sig_wait_id, SIGKILL);
    pthread_kill(allocator_id, SIGKILL);

    print_report();
    
    if(user_thread_id)
        free(user_thread_id);
    if(pdata)
        free(pdata);
   
    delete_allocator_queue();
    if(semctl(semid, IPC_RMID, 0) < 0)
        printf("Error deleting Semaphore\n");
    mem_mgr_shutdown();

	return(0);
}

void print_report()
{
    printf("\n\n################# Memory Manager report #################\n");
    printf("Number of threads\t= %d\nMemory Block Size\t= %d\nAllocation Algorithm\t= ", num_threads, get_total_memory_size());
    switch(alloc_algorithm){
        case 1: printf("FirstFit");break;
        case 2: printf("BestFit");break;
        case 3: printf("WorstFit");break;
    }
    printf("\n");
    printf("Total allocations\t= %d\n", 10*num_threads);
    printf("Succeeded allocations\t= %d\n", g_allocated);
    printf("\nFragmented Memory\t= %d\n\n", get_fragmented_mem_size());
    print_alloc_list();
    print_free_list();

    if(get_fragmented_mem_size() > 0) {
        printf("\nDefragmenting now...\n");
        defragment_memory();
        print_alloc_list();
        print_free_list();
    }
    printf("##################### End of report #######################\n\n");
}

void *sig_waiter (void *arg)
{
	sigset_t    sigterm_signal;
	int         i, signo;
    
    sigemptyset(&sigterm_signal);
    sigaddset(&sigterm_signal, SIGTERM);
    sigaddset(&sigterm_signal, SIGINT);
    
    if (sigwait(&sigterm_signal, &signo)  != 0) {
        printf ("\n  sigwait ( ) failed, exiting \n");
        exit(2);
    }
    
    printf ("Process got SIGNAL (number %d)\n\n", signo);

    pthread_kill(allocator_id, SIGKILL);

    for(i = 0; i < num_threads; i++) {
        pthread_kill(user_thread_id[i], SIGKILL);
    }
    print_report();

    if(user_thread_id)
        free(user_thread_id);
    if(pdata)
        free(pdata);

    delete_allocator_queue();
    if(semctl(semid, IPC_RMID, 0) < 0)
        printf("Error deleting Semaphore\n");
    mem_mgr_shutdown();

    exit ( 1 );
    return NULL;
}
