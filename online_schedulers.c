#include "header_files/online_schedulers.h"

int main() {
    int choice;
    printf("Choose a scheduling algorithm:\n");
    printf("1. Shortest Job First (SJF)\n");
    printf("2. Multi-level Feedback Queue (MLFQ)\n");
    printf("Enter your choice (1-2): ");
    scanf("%d", &choice);  // Read the choice
    getchar();  // Consume the newline character left in the input buffer

    switch(choice) {
        case 1:
            printf("Running Shortest Job First (SJF) scheduler\n");
            ShortestJobFirst();
            break;
        case 2:
            printf("Running Multi-level Feedback Queue (MLFQ) scheduler\n");
            int quantum0, quantum1, quantum2, boostTime;
            printf("Enter quantum for high priority queue (ms): ");
            scanf("%d", &quantum0);
            printf("Enter quantum for medium priority queue (ms): ");
            scanf("%d", &quantum1);
            printf("Enter quantum for low priority queue (ms): ");
            scanf("%d", &quantum2);
            printf("Enter priority boost time (ms): ");
            scanf("%d", &boostTime);
            MultiLevelFeedbackQueue(quantum0, quantum1, quantum2, boostTime);
            break;
        default:
            printf("Invalid choice. Exiting.\n");
            return 1;
    }

    printf("Scheduler finished. Results have been written to the appropriate CSV file.\n");
    return 0;
}
