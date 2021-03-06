/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers
#include "devices/timer.h"

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1

/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;

//Global Variables
int TasksOnBus;
int waitersH[2];
int CurrentDirection;
//Condition Variables and Lock
struct condition waitingToGO[2];
struct lock mutex;


void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive, unsigned int num_priority_send, unsigned int num_priority_receive);
void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
void getSlot(task_t task); /* task tries to use slot on the bus */
void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
void leaveSlot(task_t task); /* task release the slot */



/* initializes semaphores */ 
void init_bus(void)
{  
    random_init((unsigned int)123456789); 
    TasksOnBus = 0;
    waitersH[0] = 0;
    waitersH[1] = 0;
    waitersH[1] = 0;
    CurrentDirection = SENDER;
    cond_init(&waitingToGO[SENDER]);
    cond_init(&waitingToGO[RECEIVER]);
    lock_init(&mutex);
}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread. 
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
    unsigned int i;
    /* create sender threads */
    for(i = 0; i < num_tasks_send; i++)
        thread_create("sender_task", 1, senderTask, NULL);

    /* create receiver threads */
    for(i = 0; i < num_task_receive; i++)
        thread_create("receiver_task", 1, receiverTask, NULL);

    /* create high priority sender threads */
    for(i = 0; i < num_priority_send; i++)
       thread_create("prio_sender_task", 1, senderPriorityTask, NULL);

    /* create high priority receiver threads */
    for(i = 0; i < num_priority_receive; i++)
       thread_create("prio_receiver_task", 1, receiverPriorityTask, NULL);
}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* task tries to get slot on the bus subsystem */
void getSlot(task_t task) 
{
    lock_acquire(&mutex);
    /* wait if cannot go on bridge */
    while ((TasksOnBus == BUS_CAPACITY) || (TasksOnBus > 0 && task.direction != CurrentDirection))
    {
        if (task.priority == HIGH)
        {
            waitersH[task.direction]++;
        }
        cond_wait(&waitingToGO[task.direction], &mutex);
        if (task.priority == HIGH)
        {
            waitersH[task.direction]--;
        }
    }
    /* get on the bridge */
    TasksOnBus++;
    CurrentDirection = task.direction;
    lock_release(&mutex);
    //thread_yield();
}

/* task processes data on the bus send/receive */
void transferData(task_t task) 
{
    /* sleep the thread for random amount of time*/
    int64_t randomTicks = ((int64_t) random_ulong() % 5) + 1;
	timer_sleep(randomTicks);
}

/* task releases the slot */
void leaveSlot(task_t task) 
{
    lock_acquire(&mutex);
    TasksOnBus--;
    /* wake the tasks in current direction*/
    if (waitersH[CurrentDirection] > 0)
    {
        cond_signal(&waitingToGO[CurrentDirection], &mutex);
    }
    /* wake the tasks in current direction*/
    else if (TasksOnBus == 0)
    {
        cond_broadcast(&waitingToGO[1 - CurrentDirection], &mutex);
    }
    lock_release(&mutex);
}


