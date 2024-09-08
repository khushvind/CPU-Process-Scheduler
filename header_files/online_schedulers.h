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

// Stores the history of burst time
typedef struct {
    char *command;
    uint64_t avg_burst_time;
    int execution_count;
} ProcessHistory;

// Function prototypes
void ShortestJobFirst();
void ShortestRemainingTimeFirst();
void MultiLevelFeedbackQueue(int quantum0, int quantum1, int quantum2, int boostTime);

#define MAX_PROCESSES 100
#define INITIAL_BURST_TIME 1000
#define MAX_COMMAND_LENGTH 1024

Process p[MAX_PROCESSES];
int process_count = 0;
ProcessHistory process_history[MAX_PROCESSES];
int history_count = 0;

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

// Gets the input
int get_input(char *command) {
    fd_set readfds;
    struct timeval timeout;

    // Clear the file descriptor set
    FD_ZERO(&readfds);
    // Add stdin (fd 0) to the set
    FD_SET(STDIN_FILENO, &readfds);

    // Set timeout to 0 seconds, 0 microseconds (non-blocking)
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Check if input is available
    int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

    if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
        // Input is available, fetch it using fgets
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) != NULL) {
            // Successfully read input
            return 1;
        }
    }
    return 0;  // No input available
}

// Add process to process list
void add_process(const char* command, int current_time) {
    if (process_count < MAX_PROCESSES) {
        p[process_count].command = strdup(command);
        p[process_count].finished = false;
        p[process_count].error = false;
        p[process_count].started = false;
        p[process_count].arrival_time = current_time;
        p[process_count].burst_time = 0;
        process_count++;
    } else {
        printf("Maximum number of processes reached. Cannot add more.\n");
    }
}

// Check is input is availabe and add it.
int check_and_add_input(char *command, int *quit_func, uint64_t current_time){
    if (!get_input(command)) {
        return 0;
    }
    // Remove newline character
    command[strcspn(command, "\n")] = 0;
    if (strcmp(command, "quit") == 0) {
        *quit_func = 1;
        return 0;
    }
    add_process(command, current_time);
    return 1;
}

int find_process_history(const char* command) {
    for (int i = 0; i < history_count; i++) {
        if (strcmp(process_history[i].command, command) == 0) {
            return i;
        }
    }
    return -1;
}

void update_process_history(const char* command, uint64_t actual_burst_time) {
    int index = find_process_history(command);
    if (index != -1) {
        ProcessHistory* hist = &process_history[index];
        hist->avg_burst_time = (hist->avg_burst_time * hist->execution_count + actual_burst_time) / (hist->execution_count + 1);
        hist->execution_count++;
    } else if (history_count < MAX_PROCESSES) {
        process_history[history_count].command = strdup(command);
        process_history[history_count].avg_burst_time = actual_burst_time;
        process_history[history_count].execution_count = 1;
        history_count++;
    }
}

int get_shortest_job() {
    int shortest_job = -1;
    uint64_t shortest_time = INT64_MAX;

    for (int i = 0; i < process_count; i++) {
        if (!p[i].finished && !p[i].started && !p[i].error) {
            int hist_index = find_process_history(p[i].command);
            
            uint64_t expected_burst_time = (hist_index == -1) ? INITIAL_BURST_TIME : process_history[hist_index].avg_burst_time;
            if (expected_burst_time < shortest_time) {
                shortest_time = expected_burst_time;
                shortest_job = i;
            }
        }
    }

    return shortest_job;
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


void ShortestJobFirst() {
    char* filename = "result_online_SJF.csv";
    FILE* csv_file = fopen(filename, "w");
    fprintf(csv_file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");
    fclose(csv_file);
    char command[MAX_COMMAND_LENGTH];
    uint64_t current_time = 0;
    int quit_func = 0;
    while (!quit_func) {
        // Inner loop to keep fetching input
        while (1) {
            // Check for input and get it if available
            if (!check_and_add_input(command, &quit_func, current_time)){
                break;
            }
        }
        if (quit_func) break;
        
        // Fine the shortest job, and execute the shortest job if available
        int shortest_job = get_shortest_job();
        if (shortest_job != -1) {
            int i = shortest_job;
            p[i].start_time = current_time;
            p[i].response_time = current_time- p[i].arrival_time;
            
            uint64_t execution_start = get_current_time_ms();
            uint64_t context_switch_start = current_time;
            execute_command(&p[i]);
            uint64_t execution_end = get_current_time_ms();

            uint64_t burst_time = execution_end - execution_start;
            current_time += burst_time;
            uint64_t context_switch_end = current_time;
            
            if (!p[i].error) {
                update_process_history(p[i].command, burst_time);
            } 

            p[i].burst_time = burst_time;
            p[i].completion_time = current_time;
            p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
            p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;
            write_to_csv(filename, p[i]);
            print_context_switch(p[i].command, context_switch_start, context_switch_end);
        }
    }
    
    // The following lines clean the memory, and perform cleanup.
    for (int i = 0; i < process_count; i++) {
        free(p[i].command);
    }
    for (int i = 0; i < history_count; i++) {
        free(process_history[i].command);
    }
}

// Multi-level Feedback Queue (MLFQ), with three Queues:
#define NUM_QUEUES 3

// Defines the Queue
typedef struct {
    Process **processes;
    int size;
    int capacity;
} Queue;

// initializes the queue
Queue* createQueue(int capacity) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->processes = (Process**)malloc(sizeof(Process*) * capacity);
    queue->size = 0;
    queue->capacity = capacity;
    return queue;
}

// adds process to queue
void enqueue(Queue* queue, Process* process) {
    if (queue->size < queue->capacity) {
        queue->processes[queue->size++] = process;
    }
}

// performs dequeue
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

// Performs priority boost
void boost_priorities(Queue* queues[], uint64_t* last_boost_time, uint64_t current_time){
    for (int i = 1; i < NUM_QUEUES; i++) {
        while (queues[i]->size > 0) {
            Process* process = dequeue(queues[i]);
            enqueue(queues[0], process);  
        }
    }
    *last_boost_time = current_time;
}

// gets the priority
int get_priority(Process p, int quantum0, int quantum1, int quantum2){
    int hist_index = find_process_history(p.command);
    if (hist_index == -1) return 1;
    else if (process_history[hist_index].avg_burst_time <= quantum0) return 0;
    else if (process_history[hist_index].avg_burst_time <= quantum1) return 1;
    else return 2;
}


void MultiLevelFeedbackQueue(int quantum0, int quantum1, int quantum2, int boostTime) {
    char* filename = "result_online_MLFQ.csv";
    FILE* csv_file = fopen(filename, "w");
    fprintf(csv_file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");
    fclose(csv_file);
    int n = MAX_PROCESSES;

    Queue* queues[NUM_QUEUES];
    for (int i = 0; i < NUM_QUEUES; i++) {
        queues[i] = createQueue(n);
    }

    uint64_t current_time = 0;
    int completed = 0;
    uint64_t last_boost_time = 0;
    pid_t *pids = (pid_t *)calloc(n, sizeof(pid_t));

    char command[MAX_COMMAND_LENGTH];
    int quit_func = 0;
    while (1) {
        // Inner loop to keep fetching input
        while (1) {
            // Check for input and get it if available
            if (!check_and_add_input(command, &quit_func, current_time)){
                break;
            }
            int queue_idx = get_priority(p[process_count-1], quantum0, quantum1, quantum2);
            enqueue(queues[queue_idx], &p[process_count-1]);
        }
        if (quit_func) break;


        // Boost priority
        if (current_time - last_boost_time >= boostTime) {
            boost_priorities(queues, &last_boost_time, current_time);
        }

        int break_for_loop = 0;
        for (int q = 0; q < NUM_QUEUES && !quit_func && !break_for_loop; q++) {
            int quantum = (q == 0) ? quantum0 : (q == 1) ? quantum1 : quantum2;

            while (queues[q]->size > 0 && !quit_func && !break_for_loop) {
                // Check for input and get it if available
                if (check_and_add_input(command, &quit_func, current_time)){
                    int queue_idx = get_priority(p[process_count-1], quantum0, quantum1, quantum2);
                    enqueue(queues[queue_idx], &p[process_count-1]);
                    break_for_loop = 1;
                    break;
                }

                Process* process = dequeue(queues[q]);
                int i = process - p;
                if (!p[i].started) {
                    p[i].start_time = current_time;
                    p[i].response_time = current_time- p[i].arrival_time;
                    p[i].started = true;
                    
                    pids[i] = fork();
                    if (pids[i] == 0) {  // Child process
                        char *args[] = {"/bin/sh", "-c", process->command, NULL};
                        execvp(args[0], args);
                        exit(1);  
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
                        // process finished
                        break;
                    } 
                }
                
                if (waitpid(pids[i], &status, WNOHANG) == 0) {
                    // process did not finish within the slice
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
                // Update the history
                if (p[i].finished || p[i].error) {
                    if (p[i].finished){
                        update_process_history(p[i].command, p[i].burst_time);
                    }
                    completed++;
                    p[i].completion_time = current_time;
                    p[i].turnaround_time = p[i].completion_time - p[i].arrival_time;
                    p[i].waiting_time = p[i].turnaround_time - p[i].burst_time;
                    write_to_csv(filename, p[i]);
                } else {
                    // Drop priority
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
            if (completed >= n) {
                quit_func = 1;
                break;
            }
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