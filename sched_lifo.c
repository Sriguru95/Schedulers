#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "sched.h"

// Structure representing a task
typedef struct TaskNode {
    taskfunc task_function;     // Task function
    void* closure_data;         // Data associated with the task
    struct TaskNode* next_task; // Pointer to the next task in the stack
} TaskNode;

// Structure representing the task stack
typedef struct TaskStack {
    TaskNode* top_task;         // Top of the stack
    pthread_mutex_t lock;       // Mutex to protect the stack
    int size;                   // Current size of the stack
} TaskStack;

// Structure representing the scheduler
typedef struct scheduler {
    int num_threads;            // Number of threads in the scheduler
    int max_task_queue_length;  // Maximum capacity of the task stack
    TaskStack* task_stack;      // Task stack of the scheduler
    pthread_mutex_t cond_mutex; // Mutex for condition variable
    pthread_cond_t cond;        // Condition variable for thread synchronization
    int num_thread_sleep;       // Number of threads currently sleeping
} Scheduler;

  
// Function to push a task onto the stack 
void lifo_push(TaskStack* stack, TaskNode* task) {
    pthread_mutex_lock(&stack->lock);  
    task->next_task = stack->top_task; 
    stack->top_task = task;             
    stack->size++;                     
    pthread_mutex_unlock(&stack->lock); 
}

// Function to pop a task from the stack
TaskNode* lifo_pop(TaskStack* stack) {
    pthread_mutex_lock(&stack->lock);   
    if (stack->top_task == NULL) {      
        pthread_mutex_unlock(&stack->lock); 
        return NULL;                        
    }
    TaskNode* taskTmp = stack->top_task;  
    stack->top_task = stack->top_task->next_task; 
    stack->size--;                        
    pthread_mutex_unlock(&stack->lock);  
    return taskTmp;                       
}


void* thread_function(void* arg) {
    Scheduler* scheduler = (Scheduler*)arg; 
    while (1) {
        TaskNode* task = lifo_pop(scheduler->task_stack); 
        
        if (task != NULL) {
            taskfunc f = task->task_function;           
            (*f)(task->closure_data, scheduler);           
        } else {
           
            int threads_sleeping = scheduler->num_thread_sleep;
            if (scheduler->task_stack->size == 0 && threads_sleeping < scheduler->num_threads) {
                break; 
            }
           
            pthread_mutex_lock(&scheduler->cond_mutex);
            scheduler->num_thread_sleep++;
         
            pthread_cond_wait(&scheduler->cond, &scheduler->cond_mutex);
           
            scheduler->num_thread_sleep--;
            pthread_mutex_unlock(&scheduler->cond_mutex);
        }
    }
    return NULL; 
}

// Function to initialize the scheduler
int sched_init(int nthreads, int qlen, taskfunc f, void* closure) {
    
    if (nthreads <= 0) {
        nthreads = sysconf(_SC_NPROCESSORS_ONLN); // Number of cores of the machine
    }

    // Create the scheduler
    Scheduler* scheduler = malloc(sizeof(Scheduler));
    if (scheduler == NULL) {
        perror("Error allocating memory for the scheduler");
        return -1;
    }

    // Initialize the scheduler
    scheduler->num_threads = nthreads;
    scheduler->max_task_queue_length = qlen;
    scheduler->task_stack = malloc(sizeof(TaskStack));
    if (scheduler->task_stack == NULL) {
        perror("Error allocating memory for the task stack");
        free(scheduler);
        return -1;
    }
    scheduler->task_stack->top_task = NULL;
    scheduler->task_stack->size = 0;
    scheduler->task_stack->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    scheduler->cond_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    scheduler->cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

    // Initialize the number of sleeping threads
    scheduler->num_thread_sleep=0;

    // Add the initial task to the stack
    TaskNode* task = malloc(sizeof(TaskNode));
    if (task == NULL) {
        perror("Error allocating memory for the initial task");
        pthread_mutex_destroy(&(scheduler->task_stack->lock));
        free(scheduler->task_stack);
        free(scheduler);
        return -1;
    }
    task->task_function = f;
    task->closure_data = closure;
    task->next_task = NULL;
    lifo_push(scheduler->task_stack, task);

    // Create threads
    pthread_t* thread_array = malloc(sizeof(pthread_t) * nthreads);
    if (thread_array == NULL) {
        perror("Error allocating memory for the thread array");
	free(task);
	pthread_mutex_destroy(&(scheduler->task_stack->lock));
        free(scheduler->task_stack);
	free(scheduler);
	return -1;
    }

    for (int i = 0; i < nthreads; i++) {
       int rc = pthread_create(&(thread_array[i]), NULL, thread_function, scheduler);
       if (rc != 0) {
          fprintf(stderr, "Error creating thread: %s\n", strerror(rc));
	  free(task);
	  pthread_mutex_destroy(&(scheduler->task_stack->lock));
	  free(scheduler->task_stack);
	  free(scheduler);
	  return -1;
       }
   }

    // Join threads
    for (int i = 0; i < nthreads; i++) {
        if (pthread_join(thread_array[i], NULL) != 0) {
            perror("Error waiting for thread termination");
        }
    }

    free(task);
    pthread_mutex_destroy(&(scheduler->task_stack->lock));
    pthread_mutex_destroy(&(scheduler->cond_mutex));
    pthread_cond_destroy(&(scheduler->cond));
    free(scheduler->task_stack);
    free(thread_array);
    free(scheduler);

    return 1; 
}

// Function to add a new task to the scheduler's stack
int sched_spawn(taskfunc f, void* closure, struct scheduler *s) {
    // Check if the maximum capacity is reached or exceeded
    if (s->task_stack->size >= s->max_task_queue_length) {
        errno = EAGAIN; // Set errno to EAGAIN in case of capacity overrun
        return -1;
    }

    // If capacity is not exceeded, enqueue the task
    TaskNode* task = malloc(sizeof(TaskNode));
    if (task == NULL) {
        perror("Error allocating memory for the new task");
        return -1;
    }
    task->task_function = f;
    task->closure_data = closure;
    task->next_task = NULL;
    lifo_push(s->task_stack, task); // Enqueue the new task into the stack

    pthread_cond_signal(&s->cond); // Wake up a waiting thread

    return 1; // Return 1 to indicate successful task addition
}

