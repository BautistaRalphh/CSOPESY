#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <iostream>

#include "Process.h"

class FCFS_Scheduler {
public:
    FCFS_Scheduler(int coreCount);
    ~FCFS_Scheduler();

    void addProcess(const Process& process);    // Add process to queue
    void markCoreAvailable(int core);           // Mark core as free
    void start();                               // Start scheduler
    void stop();                                // Stop scheduler

private:
    void schedulerLoop();                       // Scheduler thread loop

    std::vector<bool> coreAvailable;
    std::queue<Process*> processQueue;

    std::mutex mtx;
    std::condition_variable cv;
    std::thread schedulerThread;
    std::atomic<bool> running;
};
