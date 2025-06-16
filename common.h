#ifndef COMMON_H
#define COMMON_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SHM_NAME "Global\\task_queue"
#define SEM_EMPTY "Global\\sem_empty"
#define SEM_FULL "Global\\sem_full"
#define SEM_MUTEX "Global\\sem_mutex"

#define BUFFER_SIZE 1000
#define TASK_BATCH_SIZE 10000

typedef struct {
    int tasks[BUFFER_SIZE];
    int head;
    int tail;
    int count;
    int total_tasks;
    int is_producer_active;
} TaskQueue;

// Function declarations
void cleanup_resources(void);
TaskQueue* init_shared_memory(void);
HANDLE create_semaphore(const char* name, int initial_value, int max_value);  // For producer
HANDLE open_semaphore(const char* name);              // For consumer

#endif // COMMON_H 