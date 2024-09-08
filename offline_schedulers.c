# include "header_files/offline_schedulers.h"


int main() {
    // Define an array of processes
    Process processes[] = {
        {.command = "sleep 1", .process_id = 1},
        {.command = "sleep 2", .process_id = 2},
        {.command = "sleep 3", .process_id = 3},
        {.command = "sleep 4", .process_id = 4},
        {.command = "sleep 5", .process_id = 5},
        {.command = "ls", .process_id = 5},
        {.command = "cd ..", .process_id = 5},
        {.command = "sgad", .process_id = 5},
    }; 
    int num_processes = sizeof(processes) / sizeof(processes[0]);

    // Run FCFS scheduler
    printf("Running First-Come First-Served (FCFS) Scheduler\n");
    FCFS(processes, num_processes);
    printf("FCFS Scheduler completed. Results written to result_offline_FCFS.csv\n\n");

    // Reset process states
    reset_processes(processes, num_processes);

    // Run Round Robin scheduler
    printf("Running Round Robin (RR) Scheduler\n");
    RoundRobin(processes, num_processes, 1000);  // 1000ms (1s) quantum
    printf("RR Scheduler completed. Results written to result_offline_RR.csv\n\n");

    // Reset process states
    reset_processes(processes, num_processes);

    // Run Multi-level Feedback Queue scheduler
    printf("Running Multi-level Feedback Queue (MLFQ) Scheduler\n");
    MultiLevelFeedbackQueue(processes, num_processes, 1000, 2000, 3000, 5000);  // 1s, 2s, 3s quanta, 5s boost
    printf("MLFQ Scheduler completed. Results written to result_offline_MLFQ.csv\n\n");

    return 0;
}

