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

class Scheduler {
public:
    Scheduler(int coreCount);
    ~Scheduler();

    void addProcess(Process* process);          // Add process to queue
    void markCoreAvailable(int core);           // Mark core as free
    void start();                               // Start scheduler
    void stop();                                // Stop scheduler

private:
    void FCFS_Loop();                       // Scheduler thread loop

    std::vector<bool> coreAvailable;
    std::queue<Process*> processQueue;

    std::mutex mtx;
    std::condition_variable cv;
    std::thread schedulerThread;
    std::atomic<bool> running;
};
