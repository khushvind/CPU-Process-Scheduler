#pragma once

//Can include any other headers as needed
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <float.h>


typedef struct {

    //This will be given by the tester function this is the process command to be scheduled
    char *command;

    //Temporary parameters for your usage can modify them as you wish
    bool finished;  //If the process is finished safely
    bool error;    //If an error occurs during execution
    uint64_t start_time;
    uint64_t completion_time;
    uint64_t turnaround_time;
    uint64_t waiting_time;
    uint64_t response_time;
    uint64_t arrival_time;
    uint64_t burst_time;

    bool started; 
    int process_id;

} Process;


// Function prototypes
void FCFS(Process p[], int n);
void RoundRobin(Process p[], int n, int quantum);
void MultiLevelFeedbackQueue(Process p[], int n, int quantum0, int quantum1, int quantum2, int boostTime);

// Helper function to reset process states
void reset_processes(Process p[], int n) {
    for (int i = 0; i < n; i++) {
        p[i].finished = false;
        p[i].error = false;
        p[i].start_time = 0;
        p[i].completion_time = 0;
        p[i].turnaround_time = 0;
        p[i].waiting_time = 0;
        p[i].response_time = 0;
        p[i].arrival_time = 0;
        p[i].burst_time = 0;
        p[i].started = false;
    }
}


// Helper function to get current time in milliseconds
uint64_t get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

void print_context_switch(char* command, uint64_t context_switch_start, uint64_t context_switch_end){
    printf("%s|%ld|%ld\n", command, context_switch_start,  context_switch_end);
}

// Writes the output to csv file
void write_to_csv(const char* filename, Process p) {
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fprintf(file, "%s,%s,%s,%lu,%lu,%lu,%lu\n",
            p.command,
            p.finished ? "Yes" : "No",
            p.error ? "Yes" : "No",
            p.burst_time,
            p.turnaround_time,
            p.waiting_time,
            p.response_time);

    fclose(file);
}

// Helper function to execute a command
void execute_command(Process *p) {
    pid_t pid = fork();

    if (pid == 0) {  // Child process
        // Execute the command
        char *args[] = {"/bin/sh", "-c", p->command, NULL};
        execvp(args[0], args);
        exit(1);  // If execvp fails
    } else if (pid > 0) {  // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            p->finished = true;
        } else {
            p->error = true;
        }
    } else {
        // Fork failed
        p->error = true;
    }
}

// First-Come, First-Served (FCFS)
void FCFS(Process p[], int n) {
    char* filename = "result_offline_FCFS.csv";
    FILE *csv_file = fopen(filename, "w");
    if (csv_file == NULL) {
        perror("Error opening CSV file");
        return;
    }
    fprintf(csv_file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");
    fclose(csv_file);

    uint64_t current_time = 0;

    for (int i = 0; i < n; i++) {
        p[i].start_time = current_time;
        p[i].arrival_time = 0;
        p[i].response_time = current_time- p[i].arrival_time;
        
        uint64_t execution_start = get_current_time_ms();
        uint64_t context_switch_start = current_time;
        execute_command(&p[i]);
        uint64_t execution_end = get_current_time_ms();

        uint64_t burst_time = execution_end - execution_start;
        current_time += burst_time;
        uint64_t context_switch_end = current_time;

        p[i].burst_time = burst_time;
        p[i].completion_time = current_time;
        p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
        p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;
        write_to_csv(filename, p[i]);
        print_context_switch(p[i].command, context_switch_start, context_switch_end);
    }
}



// Round Robin (RR)
void RoundRobin(Process p[], int n, int quantum) {
    char* filename = "result_offline_RR.csv";
    FILE *csv_file = fopen("result_offline_RR.csv", "w");
    if (csv_file == NULL) {
        perror("Error opening CSV file");
        return;
    }
    fprintf(csv_file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");
    fclose(csv_file);

    uint64_t current_time = 0;
    int completed = 0;
    pid_t *pids = (pid_t *)calloc(n, sizeof(pid_t));  // Store PIDs of child processes

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (!p[i].finished && !p[i].error) {
                if (!p[i].started) {
                    p[i].start_time = current_time;
                    p[i].arrival_time = 0;
                    p[i].response_time = current_time- p[i].arrival_time;
                    p[i].burst_time = 0;
                    p[i].started = true;

                    // Fork the process for the first time
                    pids[i] = fork();
                    if (pids[i] == 0) {  // Child process
                        char *args[] = {"/bin/sh", "-c", p[i].command, NULL};
                        execvp(args[0], args);
                        exit(1);  // If execvp fails
                    }
                }

                
                uint64_t execution_start = get_current_time_ms();
                uint64_t context_switch_start = current_time;
                uint64_t elapsed = 0;
                int status;

                // Resume the process if it was stopped
                kill(pids[i], SIGCONT);

                while (elapsed < quantum) {
                    usleep(1000);  // Sleep for 1ms
                    elapsed++;
                    if (waitpid(pids[i], &status, WNOHANG) != 0) {
                        break;
                    }
                }
                
                if (waitpid(pids[i], &status, WNOHANG) == 0) {
                    kill(pids[i], SIGSTOP);
                } else {
                    // Process finished
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        p[i].finished = true;
                    } else {
                        p[i].error = true;
                    }
                }

                uint64_t execution_end = get_current_time_ms();
                uint64_t burst_time = execution_end - execution_start;
                p[i].burst_time += burst_time;
                current_time += burst_time;
                uint64_t context_switch_end = current_time;

                if (p[i].finished || p[i].error) {
                    completed++;
                    p[i].completion_time = current_time;
                    p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
                    p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;
                    write_to_csv(filename, p[i]);
                    

                }
                print_context_switch(p[i].command, context_switch_start, context_switch_end);
            }
        }
    }

    free(pids);
}

// Multi-level Feedback Queue (MLFQ), with three Queues
#define NUM_QUEUES 3

// Defines a queue
typedef struct {
    Process **processes;
    int size;
    int capacity;
} Queue;

// Creates queue
Queue* createQueue(int capacity) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->processes = (Process**)malloc(sizeof(Process*) * capacity);
    queue->size = 0;
    queue->capacity = capacity;
    return queue;
}

// Enques the process to queue
void enqueue(Queue* queue, Process* process) {
    if (queue->size < queue->capacity) {
        queue->processes[queue->size++] = process;
    }
}

// Performs dqeueue
Process* dequeue(Queue* queue) {
    if (queue->size > 0) {
        Process* process = queue->processes[0];
        for (int i = 0; i < queue->size - 1; i++) {
            queue->processes[i] = queue->processes[i + 1];
        }
        queue->size--;
        return process;
    }
    return NULL;
}

// Helper function to boost priority
void boost_priorities(Queue* queues[], uint64_t* last_boost_time, uint64_t current_time){
    for (int i = 1; i < NUM_QUEUES; i++) {
        while (queues[i]->size > 0) {
            Process* process = dequeue(queues[i]);
            enqueue(queues[0], process);  // Move process to the highest priority queue
        }
    }
    *last_boost_time = current_time;
}


void MultiLevelFeedbackQueue(Process p[], int n, int quantum0, int quantum1, int quantum2, int boostTime) {
    char* filename = "result_offline_MLFQ.csv";
    FILE *csv_file = fopen("result_offline_MLFQ.csv", "w");
    if (csv_file == NULL) {
        perror("Error opening CSV file");
        return;
    }
    fprintf(csv_file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");
    fclose(csv_file);

    Queue* queues[NUM_QUEUES];
    for (int i = 0; i < NUM_QUEUES; i++) {
        queues[i] = createQueue(n);
    }

    uint64_t current_time = 0;
    int completed = 0;
    uint64_t last_boost_time = 0;
    pid_t *pids = (pid_t *)calloc(n, sizeof(pid_t));

    // Initially, all processes are in the highest priority queue (Q0)
    for (int i = 0; i < n; i++) {
        enqueue(queues[0], &p[i]);
    }

    while (completed < n) {
        // Boost priority
        if (current_time - last_boost_time >= boostTime) {
            boost_priorities(queues, &last_boost_time, current_time);
        }

        for (int q = 0; q < NUM_QUEUES; q++) {
            int quantum = (q == 0) ? quantum0 : (q == 1) ? quantum1 : quantum2;

            while (queues[q]->size > 0) {
                Process* process = dequeue(queues[q]);
                int i = process - p;  // Get index of the process

                if (!p[i].started) {
                    p[i].start_time = current_time;
                    p[i].arrival_time = 0;
                    p[i].response_time = current_time- p[i].arrival_time;
                    p[i].burst_time = 0;
                    p[i].started = true;
                    
                    pids[i] = fork();
                    if (pids[i] == 0) {  // Child process
                        char *args[] = {"/bin/sh", "-c", process->command, NULL};
                        execvp(args[0], args);
                        exit(1);  // If execvp fails
                    } else if (pids[i] < 0) {
                        // Fork failed
                        p[i].error = true;
                        completed++;
                        continue;
                    }
                }
                
                uint64_t execution_start = get_current_time_ms();
                uint64_t context_switch_start = current_time;
                uint64_t elapsed = 0;
                int status;
                
                kill(pids[i], SIGCONT);  // Resume the process

                while (elapsed < quantum) {
                    usleep(1000);  // Sleep for 1ms
                    elapsed++;
                    if (waitpid(pids[i], &status, WNOHANG) != 0) {
                        break;
                    }
                }
                
                if (waitpid(pids[i], &status, WNOHANG) == 0) {
                    // Process did not end within the slice
                    kill(pids[i], SIGSTOP);
                } else {
                    // Process finished
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        p[i].finished = true;
                    } else {
                        p[i].error = true;
                    }
                }
                
                uint64_t execution_end = get_current_time_ms();
                uint64_t burst_time = execution_end - execution_start;
                
                p[i].burst_time += burst_time;
                current_time += burst_time;
                uint64_t context_switch_end = current_time;

                if (p[i].finished || p[i].error) {
                    completed++;
                    p[i].completion_time = current_time;
                    p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
                    p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;
                    write_to_csv(filename, p[i]);
                } else {
                    // Drop the priority
                    if (q < NUM_QUEUES - 1) {
                        enqueue(queues[q + 1], process);
                    } else {
                        enqueue(queues[q], process);
                    }
                }
                print_context_switch(p[i].command, context_switch_start, context_switch_end);
                if (current_time - last_boost_time >= boostTime) {
                    boost_priorities(queues, &last_boost_time, current_time);
                    break;
                }
            }
            if (q > 0 && queues[0]->size > 0) break;  
            if (completed >= n) break;
        }
    }


    // The following lines clean the memory, and perform cleanup.
    for (int i = 0; i < n; i++) {
        if (pids[i] > 0) {
            kill(pids[i], SIGKILL); 
            waitpid(pids[i], NULL, 0);
        }
    }

    for (int i = 0; i < NUM_QUEUES; i++) {
        free(queues[i]->processes);
        free(queues[i]);
    }
    free(pids);
}