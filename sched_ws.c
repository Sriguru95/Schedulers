#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "sched.h"


// Structure representing a task in the task queue
typedef struct Task {
    taskfunc t;                                       // Pointer to the function to execute
    void *closure;                                    // Function arguments
    struct Task *prev;                                // Pointer to the previous task in the queue
    struct Task *next;                                // Pointer to the next task in the queue
} Task;

// Structure representing a task queue (deque)
typedef struct Deque {
    int size;                                         // Number of tasks in the queue
    Task *first;                                      // Pointer to the first task
    Task *last;                                       // Pointer to the last task
    pthread_mutex_t mutex;                            // Mutex for synchronization
} Deque;

// Structure representing thread information
typedef struct ThreadInfo {
    pthread_t thread;                                 // Thread identifier
    struct Deque *deque;                              // Task queue associated with the thread
    int sleep;                                        // Indicates whether the thread is sleeping (1) or not (0)
    struct scheduler *scheduler;                      // Reference to the scheduler
    int tasks_executed;                               // Number of tasks executed by the thread
    int successful_work_steals;                       // Number of successful work steals by the thread
    int failed_work_steals;                           // Number of failed work steals by the thread
} ThreadInfo;

// Structure representing the task scheduler
typedef struct scheduler {
    int nthreads;                                     // Number of threads
    int qlen;                                         // Minimum queue length
    ThreadInfo *threads;                              // Array of threads
    pthread_mutex_t mutex;                            // Mutex for synchronization
    int nthreads_sleep;                               // Number of threads sleeping
    int total_tasks_executed;                         // Total number of tasks executed by all threads
    int total_successful_work_steals;                 // Total number of successful work steals by all threads
    int total_failed_work_steals;                     // Total number of failed work steals by all threads
} scheduler;


// Function to display statistics for each thread
void displayStats(struct scheduler *s) {
   
    printf("\n");
    
    printf("----- Threads Statistics -----\n");
    for (int i = 0; i < s->nthreads; i++) {
        printf("Thread %d:\n", i+1);
        printf("  Tasks executed: %d\n", s->threads[i].tasks_executed);
        printf("  Successful work steals: %d\n", s->threads[i].successful_work_steals);
        printf("  Failed work steals: %d\n", s->threads[i].failed_work_steals);
    }
    printf("\n");

    printf("----- Scheduler Statistics -----\n");
    printf("Total tasks executed: %d\n", s->total_tasks_executed);
    printf("Total successful work steals: %d\n", s->total_successful_work_steals);
    printf("Total failed work steals: %d\n", s->total_failed_work_steals);
    printf("\n");

}


// Function to initialize a deque
Deque *initDeque() {
    Deque *deque = calloc(1, sizeof(Deque));
    if (deque == NULL) {
        perror("Error while creating deque");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&deque->mutex, NULL) != 0) {
        perror("Error while initializing deque mutex");
        exit(EXIT_FAILURE);
    }
    return deque;
}

// Function to free a deque
void freeDeque(Deque *deque) {
    if (deque != NULL) {
        pthread_mutex_destroy(&deque->mutex);
        free(deque);
    }
}

// Function to initialize a thread
ThreadInfo initThread(int id, struct scheduler *scheduler) {
    ThreadInfo thread;
    thread.sleep = 0;
    thread.deque = initDeque();
    thread.scheduler = scheduler;
    thread.tasks_executed = 0;
    thread.successful_work_steals = 0;
    thread.failed_work_steals = 0;
    return thread;
}

// Function to free a thread
void freeThread(ThreadInfo thread) {
    freeDeque(thread.deque);
}

// Function to destroy the scheduler and free resources
void sched_destroy(struct scheduler *s) {
    if (!s) {
        return;
    }
    for (int i = 0; i < s->nthreads; i++) {
        freeThread(s->threads[i]);
    }
    free(s->threads);
    pthread_mutex_destroy(&s->mutex);
    free(s);
}

// Add a task to the bottom of the queue
void pushBack(Deque *deque, taskfunc f, void *closure) {
    
    if (deque == NULL) {
        fprintf(stderr, "Error: uninitialized task queue\n");
        exit(EXIT_FAILURE);
    }
  
    Task *task = calloc(1, sizeof(Task));
    if (task == NULL) {
        fprintf(stderr, "Error allocating memory for a new task\n");
        exit(EXIT_FAILURE);
    }
    task->t = f;
    task->closure = closure;
    task->next = NULL;
    
    pthread_mutex_lock(&deque->mutex);
    // Add the task to the queue
    if (deque->size != 0) {
        task->prev = deque->last;
        deque->last->next = task;
        deque->last = task;
    } else {
        task->prev = NULL;
        deque->first = task;
        deque->last = task;
    }
    deque->size++;
 
    pthread_mutex_unlock(&deque->mutex);
}

// Add a task to the top of the queue
void pushFront(Deque *deque, taskfunc f, void *closure) {
    
    if (deque == NULL) {
        fprintf(stderr, "Error: uninitialized task queue\n");
        exit(EXIT_FAILURE);
    }
    
    Task *task = calloc(1, sizeof(Task));
    if (task == NULL) {
        fprintf(stderr, "Error allocating memory for a new task\n");
        exit(EXIT_FAILURE);
    }
    task->t = f;
    task->closure = closure;
  
    pthread_mutex_lock(&deque->mutex);
    
    task->next = deque->first;
    if (deque->first != NULL) {
        deque->first->prev = task;
    } else {
        // If the queue is empty, the task also becomes the last one
        deque->last = task;
    }
    deque->first = task;
    deque->size++;
    
    pthread_mutex_unlock(&deque->mutex);
}

// Remove a task from the bottom of the queue
Task *popBack(Deque *deque) {
    if (deque == NULL || deque->size == 0) {
        fprintf(stderr, "Error: empty or uninitialized task queue\n");
        exit(EXIT_FAILURE);
    }
    // Retrieve the task to return
    Task *task = deque->last;
    // Update the queue according to its size
    if (deque->size == 1) {
        deque->first = NULL;
        deque->last = NULL;
    } else {
        deque->last = deque->last->prev;
        deque->last->next = NULL;
    }
    deque->size--;
    return task;
}


// Remove a task from the top of the queue
Task *popFront(Deque *deque) {
    if (deque == NULL || deque->size == 0) {
        fprintf(stderr, "Error: empty or uninitialized task queue\n");
        exit(EXIT_FAILURE);
    }
   
    Task *task = deque->first;
   
    deque->first = task->next;
    if (deque->first == NULL) {
        deque->last = NULL;
    } else {
        deque->first->prev = NULL;
    }
    deque->size--;
    return task;
}



// Function for work stealing between threads
int workStealing(ThreadInfo *threads, int nthreads, int index) {
    for (int i = 0; i < nthreads; i++) {
        int r = (rand() % nthreads);
        if (r != index) {
            pthread_mutex_lock(&threads[r].deque->mutex);
            int t = threads[r].deque->size;
            if (t != 0) {
                threads[index].sleep = 0;
                Task *task = popFront(threads[r].deque);
                task->t(task->closure, threads[r].scheduler); // Execute the task
                pthread_mutex_unlock(&threads[r].deque->mutex);
                threads[index].successful_work_steals++;
                threads[index].scheduler->total_successful_work_steals++; // Update global stats
                return 1;
            }
            pthread_mutex_unlock(&threads[r].deque->mutex);
        }
    }
    threads[index].failed_work_steals++;
    threads[index].scheduler->total_failed_work_steals++; // Update global stats
    return 0; // Failure
}


// Function executed by each thread
void *thread_function(void* arg) {
    ThreadInfo *threadInfo = (ThreadInfo *)arg;
    int index = threadInfo - threadInfo->scheduler->threads;
    
    while (1) {
        int ws = 0;
        pthread_mutex_lock(&threadInfo->deque->mutex);
        int t = threadInfo->deque->size;
        // If the queue is empty, put the thread to sleep and try work stealing
        if (t == 0) {
            pthread_mutex_unlock(&threadInfo->deque->mutex);
            threadInfo->sleep = 1;
            while (ws == 0) {
                ws = workStealing(threadInfo->scheduler->threads, threadInfo->scheduler->nthreads, index);
                if (ws == 0) {
                    usleep(1000); // Sleep for 1 ms before retrying work stealing
                }
            }
        } else { // Otherwise, pop a task from the queue and execute it
            Task *task = popBack(threadInfo->deque);
            pthread_mutex_unlock(&threadInfo->deque->mutex);
            task->t(task->closure, threadInfo->scheduler); // Execute the popped task
            threadInfo->tasks_executed++;
            threadInfo->scheduler->total_tasks_executed++;
        }
    }
    return NULL;
}

// Function to initialize the task scheduler
int sched_init(int nthreads, int qlen, taskfunc f, void *closure) {
    // Determine the default number of threads if nthreads is -1
    if (nthreads <= 0)
        nthreads = sched_default_threads();
    

    struct scheduler *scheduler = calloc(1, sizeof(struct scheduler));
    if (scheduler == NULL) {
        perror("Error while creating scheduler [sched_init]");
        return -1;
    }

    // Initialize scheduler parameters
    scheduler->nthreads = nthreads;
    scheduler->qlen = qlen;

  
    scheduler->threads = calloc(nthreads, sizeof(ThreadInfo));
    if (scheduler->threads == NULL) {
        perror("Error while creating threads [sched_init]");
        free(scheduler);
        return -1;
    }

    // Initialize the scheduler mutex
    if (pthread_mutex_init(&scheduler->mutex, NULL) != 0) {
        perror("Error while initializing scheduler mutex [sched_init]");
        free(scheduler->threads);
        free(scheduler);
        return -1;
    }

    // Initialize the threads and their associated deques
    for (int i = 0; i < nthreads; i++) {
        scheduler->threads[i] = initThread(i, scheduler); // Assign the result of initThread
    }

    // Enqueue the initial task into the deque of the first thread
    pushFront(scheduler->threads[0].deque, f, closure);

    // Launch the threads
    for (int i = 0; i < nthreads; i++) {
        int rc = pthread_create(&scheduler->threads[i].thread, NULL, thread_function, (void *)&scheduler->threads[i]);
        if (rc != 0) {
            fprintf(stderr, "Error creating thread: %s\n", strerror(rc));
            sched_destroy(scheduler);
            return -1;
        }
    }

    // Wait for all threads to sleep
    while (scheduler->nthreads_sleep != nthreads) {
        usleep(10);
        scheduler->nthreads_sleep = 0;
        for (int i = 0; i < nthreads; i++) {
            if (scheduler->threads[i].sleep == 1)
                scheduler->nthreads_sleep++;
        }
    }

    // to display statistics
    displayStats(scheduler);

    return 1;
}

// Function to create a new task in the scheduler
int sched_spawn(taskfunc f, void *closure, struct scheduler *s) {
    if (!s) {
        fprintf(stderr, "Error: NULL scheduler pointer [sched_spawn]");
        return -1;
    }
    pthread_t thread = pthread_self();
    int id;
    // Find the ID of the current thread
    for (id = 0; id < s->nthreads; id++) {
        if (s->threads[id].thread == thread) {
            break;
        }
    }
    // Check if the ID was found
    if (id == s->nthreads) {
        fprintf(stderr, "Error: Thread ID not found in scheduler [sched_spawn]");
        return -1; 
    }
    // Enqueue the task into the appropriate deque
    pushBack(s->threads[id].deque, f, closure);
    return 0;
}

