#ifndef TASK_H
#define TASK_H

#include <stdbool.h>
#include <time.h>

#define MAX_TASKS 100
#define TIME_SLICE_MS 100

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_FINISHED
} TaskState;

typedef enum {
    SCHED_PRIORITY,
    SCHED_ROUND_ROBIN,
    SCHED_FCFS
} SchedulerType;

typedef struct {
    int id;
    int duration_ms;
    int remaining_ms;
    int priority;
    TaskState state;
    time_t arrival_time;
    time_t finish_time;
} Task;

extern Task tasks[MAX_TASKS];
extern int task_count;
extern SchedulerType sched_type;

void create_task(int id, int duration_ms, int priority);
void print_task_stats();

#endif