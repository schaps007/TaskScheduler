#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pthread.h>

extern pthread_mutex_t lock;

void* scheduler(void* arg);
void* unblocker(void* arg);
void set_scheduler_type(int type);
void execute_task(int task_idx, unsigned int* seed);

#endif