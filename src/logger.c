#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "logger.h"

FILE* log_file;

void init_logger() {
    log_file = fopen("scheduler.log", "w");
    if (!log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
}

void log_message(const char* message) {
    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strlen(time_str)-1] = '\0'; // Remove newline
    fprintf(log_file, "[%s] %s\n", time_str, message);
    printf("[%s] %s\n", time_str, message);
}

void close_logger() {
    fclose(log_file);
}