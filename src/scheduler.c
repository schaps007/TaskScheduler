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
    } else if(type==2){
        sched_type = SCHED_ROUND_ROBIN;
    } else{
        sched_type = SCHED_FCFS;
    } 
}

// Round Robin index tracker
static int rr_index = 0;

void* unblocker(void* arg) {
    (void)arg;
    while (1) {
        bool all_done = true;
        pthread_mutex_lock(&lock);
        
        for (int i = 0; i < task_count; i++) {
            if (tasks[i].state != TASK_FINISHED) all_done = false;
            if (tasks[i].state == TASK_BLOCKED && rand() % 3 == 0) {
                tasks[i].state = TASK_READY;
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
    while (1) {
        bool all_done = true;
        pthread_mutex_lock(&lock);

        if (sched_type == SCHED_PRIORITY) {
            // Find READY task with highest priority (lowest priority number)
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
            // Find next READY task starting from rr_index
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
                // No READY tasks found, check if all done or waiting for unblocker
                for (int i = 0; i < task_count; i++) {
                    if (tasks[i].state != TASK_FINISHED) all_done = false;
                }
            }

        } else if (sched_type == SCHED_FCFS) {
            // FCFS: Run first READY task in order of ID until block/finish
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
                // Run task[i] to either block or finish (can split time slice if needed)
                tasks[i].state = TASK_RUNNING;

                // FCFS usually runs until finish or blocking without slicing
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
                    // If task didn't finish or block (unlikely here), mark READY again
                    tasks[i].state = TASK_READY;
                }
            }
        }

        pthread_mutex_unlock(&lock);
        if (all_done) break;
    }
    return NULL;
}
