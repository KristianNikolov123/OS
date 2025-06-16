#include "common.h"

TaskQueue* queue;
HANDLE sem_empty, sem_full, sem_mutex;
HANDLE hMapFile;

void cleanup_resources(void) {
    if (queue) {
        UnmapViewOfFile(queue);
    }
    if (hMapFile) {
        CloseHandle(hMapFile);
    }
    CloseHandle(sem_empty);
    CloseHandle(sem_full);
    CloseHandle(sem_mutex);
}

TaskQueue* init_shared_memory(void) {
    hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SHM_NAME);

    if (hMapFile == NULL) {
        printf("OpenFileMapping failed (%lu)\n", GetLastError());
        exit(1);
    }

    queue = (TaskQueue*)MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(TaskQueue));

    if (queue == NULL) {
        printf("MapViewOfFile failed (%lu)\n", GetLastError());
        CloseHandle(hMapFile);
        exit(1);
    }

    return queue;
}

HANDLE open_semaphore(const char* name) {
    HANDLE sem = OpenSemaphore(
        SEMAPHORE_ALL_ACCESS,
        FALSE,
        name);

    if (sem == NULL) {
        printf("OpenSemaphore failed (%lu)\n", GetLastError());
        exit(1);
    }
    return sem;
}

int main() {
    // Initialize shared memory and semaphores
    queue = init_shared_memory();
    sem_empty = open_semaphore(SEM_EMPTY);
    sem_full = open_semaphore(SEM_FULL);
    sem_mutex = open_semaphore(SEM_MUTEX);

    printf("Consumer started\n");

    while (queue->is_producer_active || queue->count > 0) {
        WaitForSingleObject(sem_full, INFINITE);   // Wait for a full slot
        WaitForSingleObject(sem_mutex, INFINITE);  // Enter critical section

        if (queue->count > 0) {
            // Get task from queue
            int task = queue->tasks[queue->head];
            queue->head = (queue->head + 1) % BUFFER_SIZE;
            queue->count--;

            ReleaseSemaphore(sem_mutex, 1, NULL);  // Leave critical section
            ReleaseSemaphore(sem_empty, 1, NULL);  // Signal that a slot is empty

            // Process the task (in this case, just print it)
            printf("Processing task: %d\n", task);
        } else {
            ReleaseSemaphore(sem_mutex, 1, NULL);
        }
    }

    printf("Consumer finished\n");
    cleanup_resources();
    return 0;
} 