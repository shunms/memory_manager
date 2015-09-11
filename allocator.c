/*--------------- allocator.c ---------------*/
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

int qid;
extern long allocator_channel;
extern int num_threads;
extern pthread_t *user_thread_id;


extern void mem_mgr_free(void *ptr, int thread_id);
extern void *mem_mgr_malloc(unsigned int size, int thread_id);

/*********** Constant Declaration **********/
#define MSG_Q_KEY       1000

/* cmds accepted by allocator */
#define ALLOCATE_MEM    0
#define FREE_MEM        1

typedef struct {
    long dest;      /* mtype */
    long source;
    int command;
    union {
        unsigned int size;
        unsigned int addr;
    }u;
} msg_pkt_t;

long get_user_channel(pthread_t user_id)
{
    long i;
    for(i = 0; i < num_threads; i++){
        if(user_id == user_thread_id[i])
            return(i+1);
    }
    /* Invalid User */
    return(-1);
}

void create_allocator_queue()
{
    if((qid = msgget((key_t)MSG_Q_KEY, IPC_CREAT|0666)) == -1) {
        printf("!!!Error: Can not create the Message Q\n");
        exit(2);
    }
}

void delete_allocator_queue()
{
    if(msgctl(qid, IPC_RMID, 0) < 0){
        printf("Error Removing the Message q\n");
    }
    printf("Allocator Q deleted\n");
}

/*********** Allocator Thread Program **********/
void *allocator_thread(void * arg)
{
    msg_pkt_t recv_msg, reply_msg;
    
    while(1) {
        /* Read the message from the Q */
        if(msgrcv(qid, &recv_msg, sizeof(recv_msg), allocator_channel, 0) == -1)
        {
            printf("memory_malloc: Can not recv message\n");
            exit(1);
        }

        reply_msg.dest = recv_msg.source;
        reply_msg.source = allocator_channel;
        reply_msg.command = recv_msg.command;
        
        switch(recv_msg.command){
            case ALLOCATE_MEM:
                reply_msg.u.addr = (unsigned int)mem_mgr_malloc(recv_msg.u.size, (int)recv_msg.source);
                break;
            case FREE_MEM:
                reply_msg.u.addr = (unsigned int) recv_msg.u.addr;
                
                if(((unsigned char *)recv_msg.u.addr) != NULL){
                    mem_mgr_free((unsigned char *)recv_msg.u.addr, (int)recv_msg.source); 
                    reply_msg.u.addr = (unsigned int) NULL;
                }
                break;
            default:
                printf("allocator_thread: Error in processing commnand\n");
                exit(2);
                break;
        }

        /* Post the reply in the Q */
        if(msgsnd(qid, &reply_msg, sizeof(reply_msg), 0) == -1) {
            printf("memory_malloc: Can not send message to allocator\n");
            exit(1);
        }
    }

}

/* post message to allocator to malloc */
void * request_allocator_to_malloc(int size)
{
    msg_pkt_t my_msg, reply_msg;
    long user_channel = get_user_channel(pthread_self());
    
    /* Construt a message pkt */
    my_msg.dest = (long)allocator_channel;
    my_msg.source = user_channel;
    my_msg.command = ALLOCATE_MEM;
    my_msg.u.size = size;
    
    
    /* Post a message in to the message Q to allocate memory */
    if(msgsnd(qid, &my_msg, sizeof(my_msg), 0) == -1) {
        printf("memory_malloc: Can not send message to allocator\n");
        exit(1);
    }

    /* Wait for the message with reply */
    if(msgrcv(qid, &reply_msg, sizeof(reply_msg), user_channel, 0) == -1) {
        printf("memory_malloc: Can not recv message\n");
        exit(2);
    }

    if((unsigned char *)reply_msg.u.addr != NULL)
        return((void *) reply_msg.u.addr);
    else
        return(NULL);
    
}

/* post message to allocator to free */
int request_allocator_to_free(void *ptr)
{
    msg_pkt_t my_msg, reply_msg;
    long user_channel = get_user_channel(pthread_self());
    
    /* Construt a message pkt */
    my_msg.dest = allocator_channel;
    my_msg.source = user_channel;
    my_msg.command = FREE_MEM;
    my_msg.u.addr = (unsigned int)ptr;
    
    /* Post a message in to the message Q to allocate memory */
    if(msgsnd(qid, &my_msg, sizeof(my_msg), 0) == -1) {
        printf("memory_malloc: Can not send message to allocator\n");
        exit(1);
    }

    /* Wait for the message with reply */
    if(msgrcv(qid, &reply_msg, sizeof(reply_msg), user_channel, 0) == -1) {
        printf("memory_malloc: Can not recv message\n");
        exit(2);
    }

    if((unsigned char *)reply_msg.u.addr != (unsigned char *) ptr)
        return(0);
    else
        return(-1);
}
