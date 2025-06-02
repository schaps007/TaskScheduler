#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include "task.h"
#include "scheduler.h"
#include "logger.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void set_scheduler_type(int type) {
    if (type == 1){
        sched_type = SCHED_PRIORITY;
    } else if(type == 2){
        sched_type = SCHED_ROUND_ROBIN;
    } else if (type == 4) {
        sched_type = SCHED_MLFQ;
    } else {
        sched_type = SCHED_FCFS;
    }
}

// Round Robin index tracker
static int rr_index = 0;

#define NUM_QUEUES 3
#define QUEUE_CAPACITY 100
int queue_levels[NUM_QUEUES][QUEUE_CAPACITY];
int queue_counts[NUM_QUEUES] = {0};

// MLFQ: track which queue a task is currently in (index by task id)
int task_queue_level[MAX_TASKS]; // Assume MAX_TASKS is defined in scheduler.h or task.h

void enqueue(int level, int task_id) {
    if (queue_counts[level] < QUEUE_CAPACITY) {
        queue_levels[level][queue_counts[level]++] = task_id;
    }
}

int dequeue(int level) {
    if (queue_counts[level] == 0) return -1;
    int id = queue_levels[level][0];
    for (int i = 1; i < queue_counts[level]; i++)
        queue_levels[level][i-1] = queue_levels[level][i];
    queue_counts[level]--;
    return id;
}

void* unblocker(void* arg) {
    (void)arg;
    while (1) {
        bool all_done = true;
        pthread_mutex_lock(&lock);
        
        for (int i = 0; i < task_count; i++) {
            if (tasks[i].state != TASK_FINISHED) all_done = false;

            if (tasks[i].state == TASK_BLOCKED && rand() % 3 == 0) {
                tasks[i].state = TASK_READY;
                // For MLFQ, when unblocked, re-enqueue at the same priority level
                if (sched_type == SCHED_MLFQ) {
                    enqueue(task_queue_level[tasks[i].id], tasks[i].id);
                }
                char msg[50];
                sprintf(msg, "Task %d unblocked", tasks[i].id);
                log_message(msg);
            }
        }
        
        pthread_mutex_unlock(&lock);
        if (all_done) break;
        usleep(200 * 1000);
    }
    return NULL;
}

void* scheduler(void* arg) {
    (void)arg;

    // For MLFQ: initialize all ready tasks into the highest priority queue 0
    if (sched_type == SCHED_MLFQ) {
        pthread_mutex_lock(&lock);
        for (int i = 0; i < task_count; i++) {
            task_queue_level[tasks[i].id] = 0;
            if (tasks[i].state == TASK_READY) {
                enqueue(0, tasks[i].id);
            }
        }
        pthread_mutex_unlock(&lock);
    }

    while (1) {
        bool all_done = true;
        pthread_mutex_lock(&lock);

        if (sched_type == SCHED_PRIORITY) {
            // existing priority scheduling code (unchanged)
            int highest_priority_index = -1;
            for (int i = 0; i < task_count; i++) {
                if (tasks[i].state == TASK_READY) {
                    if (highest_priority_index == -1 ||
                        tasks[i].priority < tasks[highest_priority_index].priority) {
                        highest_priority_index = i;
                    }
                } else if (tasks[i].state != TASK_FINISHED) {
                    all_done = false;
                }
            }

            if (highest_priority_index != -1) {
                all_done = false;
                int i = highest_priority_index;
                tasks[i].state = TASK_RUNNING;

                int exec_time = (tasks[i].remaining_ms < TIME_SLICE_MS) ?
                                tasks[i].remaining_ms : TIME_SLICE_MS;

                char msg[100];
                sprintf(msg, "Task %d running for %dms (Priority: %d)",
                        tasks[i].id, exec_time, tasks[i].priority);
                log_message(msg);

                pthread_mutex_unlock(&lock);
                usleep(exec_time * 1000);
                pthread_mutex_lock(&lock);

                tasks[i].remaining_ms -= exec_time;
                tasks[i].state = TASK_READY;

                if (tasks[i].remaining_ms <= 0) {
                    tasks[i].state = TASK_FINISHED;
                    tasks[i].finish_time = time(NULL);
                    sprintf(msg, "Task %d finished", tasks[i].id);
                    log_message(msg);
                } else if (rand() % 5 == 0) {
                    tasks[i].state = TASK_BLOCKED;
                    sprintf(msg, "Task %d blocked", tasks[i].id);
                    log_message(msg);
                }
            }

        } else if (sched_type == SCHED_ROUND_ROBIN) {
            // existing Round Robin code (unchanged)
            int start_index = rr_index;
            bool found = false;
            for (int checked = 0; checked < task_count; checked++) {
                int i = (start_index + checked) % task_count;
                if (tasks[i].state == TASK_READY) {
                    found = true;
                    rr_index = (i + 1) % task_count;  // Next round starts after this task
                    all_done = false;

                    tasks[i].state = TASK_RUNNING;

                    int exec_time = (tasks[i].remaining_ms < TIME_SLICE_MS) ?
                                    tasks[i].remaining_ms : TIME_SLICE_MS;

                    char msg[100];
                    sprintf(msg, "Task %d running for %dms (Priority: %d)",
                            tasks[i].id, exec_time, tasks[i].priority);
                    log_message(msg);

                    pthread_mutex_unlock(&lock);
                    usleep(exec_time * 1000);
                    pthread_mutex_lock(&lock);

                    tasks[i].remaining_ms -= exec_time;
                    tasks[i].state = TASK_READY;

                    if (tasks[i].remaining_ms <= 0) {
                        tasks[i].state = TASK_FINISHED;
                        tasks[i].finish_time = time(NULL);
                        sprintf(msg, "Task %d finished", tasks[i].id);
                        log_message(msg);
                    } else if (rand() % 5 == 0) {
                        tasks[i].state = TASK_BLOCKED;
                        sprintf(msg, "Task %d blocked", tasks[i].id);
                        log_message(msg);
                    }
                    break;
                } else if (tasks[i].state != TASK_FINISHED) {
                    all_done = false;
                }
            }

            if (!found) {
                for (int i = 0; i < task_count; i++) {
                    if (tasks[i].state != TASK_FINISHED) all_done = false;
                }
            }

        } else if (sched_type == SCHED_FCFS) {
            // existing FCFS code (unchanged)
            int i;
            for (i = 0; i < task_count; i++) {
                if (tasks[i].state == TASK_READY) {
                    all_done = false;
                    break;
                } else if (tasks[i].state != TASK_FINISHED) {
                    all_done = false;
                }
            }

            if (i < task_count) {
                tasks[i].state = TASK_RUNNING;

                int exec_time = tasks[i].remaining_ms;

                char msg[100];
                sprintf(msg, "Task %d running for %dms (Priority: %d)",
                        tasks[i].id, exec_time, tasks[i].priority);
                log_message(msg);

                pthread_mutex_unlock(&lock);
                usleep(exec_time * 1000);
                pthread_mutex_lock(&lock);

                tasks[i].remaining_ms -= exec_time;

                if (tasks[i].remaining_ms <= 0) {
                    tasks[i].state = TASK_FINISHED;
                    tasks[i].finish_time = time(NULL);
                    sprintf(msg, "Task %d finished", tasks[i].id);
                    log_message(msg);
                } else if (rand() % 5 == 0) {
                    tasks[i].state = TASK_BLOCKED;
                    sprintf(msg, "Task %d blocked", tasks[i].id);
                    log_message(msg);
                } else {
                    tasks[i].state = TASK_READY;
                }
            }

        } else if (sched_type == SCHED_MLFQ) {
            // Multi-Level Feedback Queue Scheduling
            all_done = true;
            int selected_task = -1;
            int selected_level = -1;

            // Find first non-empty queue (highest priority queue with tasks)
            for (int level = 0; level < NUM_QUEUES; level++) {
                if (queue_counts[level] > 0) {
                    selected_level = level;
                    selected_task = dequeue(level);
                    break;
                }
            }

            // Check if all tasks finished
            for (int i = 0; i < task_count; i++) {
                if (tasks[i].state != TASK_FINISHED) {
                    all_done = false;
                    break;
                }
            }

            if (selected_task != -1) {
                all_done = false;
                int i = selected_task;

                // Only schedule if task is ready
                if (tasks[i].state == TASK_READY) {
                    tasks[i].state = TASK_RUNNING;

                    // Use smaller time slice for lower priority queues (optional)
                    int base_slice = TIME_SLICE_MS;
                    int exec_time = tasks[i].remaining_ms < base_slice ? tasks[i].remaining_ms : base_slice;

                    // You can adjust time slice per queue level here:
                    // exec_time = exec_time >> selected_level;  // e.g. halve slice per lower priority

                    char msg[100];
                    sprintf(msg, "Task %d running for %dms at queue level %d",
                            tasks[i].id, exec_time, selected_level);
                    log_message(msg);

                    pthread_mutex_unlock(&lock);
                    usleep(exec_time * 1000);
                    pthread_mutex_lock(&lock);

                    tasks[i].remaining_ms -= exec_time;

                    // If finished
                    if (tasks[i].remaining_ms <= 0) {
                        tasks[i].state = TASK_FINISHED;
                        tasks[i].finish_time = time(NULL);
                        sprintf(msg, "Task %d finished", tasks[i].id);
                        log_message(msg);
                    }
                    // Random blocking to simulate IO or other waits
                    else if (rand() % 5 == 0) {
                        tasks[i].state = TASK_BLOCKED;
                        sprintf(msg, "Task %d blocked", tasks[i].id);
                        log_message(msg);
                    } else {
                        tasks[i].state = TASK_READY;
                        // Demote task to lower priority queue if not already at lowest
                        if (selected_level < NUM_QUEUES - 1) {
                            task_queue_level[tasks[i].id] = selected_level + 1;
                            enqueue(selected_level + 1, tasks[i].id);
                        } else {
                            // Re-enqueue in same queue if already lowest
                            enqueue(selected_level, tasks[i].id);
                        }
                    }
                } else {
                    // If task not READY (blocked or running), just skip and continue
                }
            }
        }

        pthread_mutex_unlock(&lock);
        if (all_done) break;
    }
    return NULL;
}
