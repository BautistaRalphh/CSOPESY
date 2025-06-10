#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

#include "Process.h"


class Scheduler {
public:
    Scheduler(int coreCount);
    ~Scheduler();

    void addProcess(const Process& process);   // Add process to queue
    void markCoreAvailable(int coreId);        // Free a core
    void start();                              // Start scheduler thread
    void stop();                               // Stop scheduler

private:
    void schedulerLoop();                      // Dispatch loop

    std::queue<Process> processQueue;
    std::vector<bool> coreAvailable;
    std::mutex mtx;
    std::condition_variable cv;
    std::thread schedulerThread;
    bool running;
};

#endif