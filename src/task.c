#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "task.h"
#include "logger.h"

Task tasks[MAX_TASKS];
int task_count = 0;
SchedulerType sched_type = SCHED_PRIORITY;

void create_task(int id, int duration_ms, int priority) {
    if (task_count >= MAX_TASKS) {
        log_message("Maximum task limit reached");
        return;
    }
    tasks[task_count].queue_level = 0;
    if (sched_type == SCHED_MLFQ) {
        enqueue(0, task_count);
    }

    tasks[task_count] = (Task){
        .id = id,
        .duration_ms = duration_ms,
        .remaining_ms = duration_ms,
        .priority = priority,
        .state = TASK_READY,
        .arrival_time = time(NULL)
    };
    
    char msg[100];
    sprintf(msg, "Task %d created: Duration=%dms, Priority=%d", id, duration_ms, priority);
    log_message(msg);
    
    task_count++;
}

void print_task_stats() {
    printf("\n=== Task Summary ===\n");
    for (int i = 0; i < task_count; i++) {
        double turnaround = difftime(tasks[i].finish_time, tasks[i].arrival_time);
        printf("Task %d | Priority: %d | Turnaround: %.2fs | Status: %s\n",
               tasks[i].id,
               tasks[i].priority,
               turnaround,
               tasks[i].state == TASK_FINISHED ? "Finished" : "Incomplete");
    }
}