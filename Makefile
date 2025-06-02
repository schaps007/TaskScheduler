CC = gcc
CFLAGS = -Wall -Wextra -pthread
INCLUDES = -Iinclude
SRC = src/main.c src/task.c src/scheduler.c src/logger.c
TARGET = scheduler

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -f $(TARGET) scheduler.log
	rm -f $(TARGET) scheduler.exe

run: $(TARGET)
	./$(TARGET)