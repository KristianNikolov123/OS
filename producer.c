#include "common.h"

TaskQueue* queue;
HANDLE sem_empty, sem_full, sem_mutex;
HANDLE hMapFile;

// Console control handler for graceful exit
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        printf("\nProducer received CTRL+C. Cleaning up resources and exiting.\n");
        fflush(stdout);
        // Signal to consumers that producer is no longer active
        if (queue) {
            queue->is_producer_active = 0;
        }
        cleanup_resources();
        exit(0);
    }
    return FALSE;
}

void cleanup_resources(void) {
    if (queue) {
        UnmapViewOfFile(queue);
    }
    if (hMapFile) {
        CloseHandle(hMapFile);
    }
    if (sem_empty) {
        CloseHandle(sem_empty);
    }
    if (sem_full) {
        CloseHandle(sem_full);
    }
    if (sem_mutex) {
        CloseHandle(sem_mutex);
    }
}

TaskQueue* init_shared_memory(void) {
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(TaskQueue),
        SHM_NAME);

    if (hMapFile == NULL) {
        printf("CreateFileMapping failed (%lu)\n", GetLastError());
        fflush(stdout);
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
        fflush(stdout);
        CloseHandle(hMapFile);
        exit(1);
    }

    return queue;
}

HANDLE create_semaphore(const char* name, int initial_value, int max_value) {
    HANDLE sem = CreateSemaphore(
        NULL,
        initial_value,
        max_value,
        name);

    if (sem == NULL) {
        printf("CreateSemaphore failed (%lu)\n", GetLastError());
        fflush(stdout);
        exit(1);
    }
    return sem;
}

int main() {
    // Set console control handler
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        printf("Could not set control handler! (%lu)\n", GetLastError());
        fflush(stdout);
        return 1;
    }

    // Initialize shared memory and semaphores
    queue = init_shared_memory();
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->total_tasks = 0;
    queue->is_producer_active = 1;

    sem_empty = create_semaphore(SEM_EMPTY, BUFFER_SIZE, BUFFER_SIZE);
    sem_full = create_semaphore(SEM_FULL, 0, BUFFER_SIZE);
    sem_mutex = create_semaphore(SEM_MUTEX, 1, 1);

    LARGE_INTEGER frequency;
    LARGE_INTEGER start_time, current_time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_time);
    int tasks_since_last_report = 0;

    while (1) {
        // Generate a new task (simple integer in this case)
        int task = queue->total_tasks + 1;

        WaitForSingleObject(sem_empty, INFINITE);  // Wait for empty slot
        WaitForSingleObject(sem_mutex, INFINITE);  // Enter critical section

        // Add task to queue
        queue->tasks[queue->tail] = task;
        queue->tail = (queue->tail + 1) % BUFFER_SIZE;
        queue->count++;
        queue->total_tasks++;

        ReleaseSemaphore(sem_mutex, 1, NULL);  // Leave critical section
        ReleaseSemaphore(sem_full, 1, NULL);   // Signal that a slot is full

        tasks_since_last_report++;

        // Report speed every TASK_BATCH_SIZE tasks
        if (tasks_since_last_report >= TASK_BATCH_SIZE) {
            QueryPerformanceCounter(&current_time);
            double elapsed = (double)(current_time.QuadPart - start_time.QuadPart) / frequency.QuadPart;
            double tasks_per_sec = TASK_BATCH_SIZE / elapsed;
            double ms_per_task = (elapsed * 1000) / TASK_BATCH_SIZE;
            
            printf("%.2f tasks/sec, %.2f ms/task\n", tasks_per_sec, ms_per_task);
            fflush(stdout); // Flush output to log file
            
            tasks_since_last_report = 0;
            start_time = current_time;
        }
    }

    // This part should not be reached in the infinite loop unless an error occurs
    cleanup_resources();
    return 0;
} 