# CPU-Process-Scheduler

## Overview
This project implements various CPU scheduling algorithms in C, with both offline and online scheduling modes. The purpose of the project is to explore the intricacies of CPU scheduling through First-Come, First-Served (FCFS), Round Robin (RR), Multi-level Feedback Queue (MLFQ) and Shortest Job First (SJF) algorithms. The results of each scheduling run are printed to CSV files for further analysis.

## Features
### Offline Scheduling: All processes are provided at the start, and the scheduler runs without interruption.
- First-Come, First-Served (FCFS): Processes are executed in the order they arrive.
- Round Robin (RR): Each process gets a time slice and switches contexts after each slice.
- Multi-level Feedback Queue (MLFQ): Uses three priority levels with feedback for dynamic adjustment.

### Online Scheduling: Processes are added dynamically during execution, and new processes can arrive at any time.
- Multi-level Feedback Queue (MLFQ) with Adaptive Features: Automatically adjusts the priority of a process based on its previous burst time.
- Shortest Job First (SJF): Processes are executed based on their expected burst times, with real-time updates.