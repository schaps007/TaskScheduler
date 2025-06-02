### Multithreaded Task Scheduler in C

This project implements a multithreaded CPU task scheduler in C, simulating three fundamental scheduling strategies used in operating systems:

- Priority Scheduling-  Tasks with higher priority (lower priority number) are scheduled first.
- Round Robin (RR)- Tasks are scheduled in a cyclic order, running each task for a fixed time slice (quantum) before moving to the next.
- First-Come, First-Served (FCFS)- Tasks are executed in the order they arrive, without preemption.

## Key Features
- Task Modeling: Each task has an ID, priority, total required runtime, and internal state `(READY, RUNNING, BLOCKED, FINISHED)`.

**Multithreaded Simulation:**
- One thread simulates the scheduler. 
- Another thread periodically unblocks blocked tasks, mimicking I/O completion or other asynchronous events.
- Dynamic Behavior: Tasks can randomly become blocked and are later unblocked by the "unblocker" thread, introducing real-world unpredictability.
- Thread-Safe Execution: All shared state is protected with `pthread_mutex_t` to avoid race conditions.
- Logging: Events such as task execution, blocking, unblocking, and completion are logged in real-time
