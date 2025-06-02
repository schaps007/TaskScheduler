#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "task.h"
#include "scheduler.h"
#include "logger.h"

int main() {
    pthread_mutex_init(&lock, NULL);
    init_logger();
    srand(time(NULL));

    printf("Select scheduler type:\n");
    printf("1. Priority\n2. Round Robin\n3. FCFS\n> ");
    int choice;
    scanf("%d", &choice);
    set_scheduler_type(choice);

    int n;
    printf("Enter number of tasks: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        int dur = 200 + rand() % 400;
        int pri = 1 + rand() % 3;
        create_task(i, dur, pri);
    }

    pthread_t sched_thread, unblock_thread;
    pthread_create(&sched_thread, NULL, scheduler, NULL);
    pthread_create(&unblock_thread, NULL, unblocker, NULL);

    pthread_join(sched_thread, NULL);
    pthread_join(unblock_thread, NULL);

    print_task_stats();
    pthread_mutex_destroy(&lock);
    close_logger();
    return 0;
}