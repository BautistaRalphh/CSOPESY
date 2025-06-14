#pragma once

#include "Process.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <iostream>

class Process;

enum class SchedulerAlgorithmType {
    NONE,
    FCFS,
};

class Scheduler {
public:
    Scheduler(int coreCount);
    ~Scheduler();

    void addProcess(Process* process);          // Add process to queue
    void markCoreAvailable(int core);           // Mark core as free
    void start();                               // Start scheduler
    void stop();                                // Stop scheduler
    void resetCoreStates();
    void setAlgorithmType(SchedulerAlgorithmType type);
    SchedulerAlgorithmType getAlgorithmType() const;

    int getTotalCores() const;
    int getCoresUsed() const;
    int getCoresAvailable() const;
    double getCpuUtilization() const;
    bool isRunning() const;

private:
    void runSchedulingLoop();                      // Scheduler thread loop

    // Private functions for each algorithm's logic
    void _runFCFSLogic(std::unique_lock<std::mutex>& lock);
    // void _runRoundRobinLogic(std::unique_lock<std::mutex>& lock);

    std::vector<bool> coreAvailable;
    std::queue<Process*> processQueue;

    mutable std::mutex mtx;
    mutable std::condition_variable cv;
    std::thread schedulerThread;
    std::atomic<bool> running;
    SchedulerAlgorithmType currentAlgorithm;
};
