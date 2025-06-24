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
#include <map>
#include <chrono>
#include <cstdint> 

class Process;

enum class SchedulerAlgorithmType {
    NONE,
    fcfs,
    rr
};

struct SleepingProcess {
    Process* process;
    long long wakeUpTime; 
    int assignedCoreId;   
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
    long long getSimulatedTime() const;
    void advanceSimulatedTime(long long deltaTime);
    void checkSleepingProcesses();

    void setDelaysPerExecution(uint32_t delays) { delaysPerExecution = delays; }

    uint32_t getDelaysPerExecution() const { return delaysPerExecution; }
    void setQuantumCycles(uint32_t quantum) { quantumCycles = quantum; }
private:
    void runSchedulingLoop();                   // Scheduler thread loop

    // Private functions for each algorithm's logic
    void _runFCFSLogic(std::unique_lock<std::mutex>& lock);
    void _runRoundRobinLogic(std::unique_lock<std::mutex>& lock);

    void _markCoreAvailableUnlocked(int core);
    void _addProcessUnlocked(Process* process);
    void _setAlgorithmTypeUnlocked(SchedulerAlgorithmType type);
    SchedulerAlgorithmType _getAlgorithmTypeUnlocked() const;
    int _getCoresUsedUnlocked() const;
    int _getCoresAvailableUnlocked() const;
    long long _getSimulatedTimeUnlocked() const;
    void _advanceSimulatedTimeUnlocked(long long deltaTime);
    void _checkSleepingProcessesUnlocked();
    bool _sleepQuickScan() const; 

    std::string getCurrentTimestamp(); 

    int numCores;
    std::vector<bool> coreAvailable;
    std::vector<std::queue<Process*>> processQueues;
    std::queue<Process*> globalQueue;
    int nextCoreForNewProcess;                  

    mutable std::mutex mtx;
    mutable std::condition_variable cv;
    std::unique_ptr<std::thread> schedulerThread;
    std::atomic<bool> running;
    SchedulerAlgorithmType currentAlgorithm; 

    std::vector<SleepingProcess> sleepingProcesses;
    long long simulatedTime;
    std::vector<Process*> coreAssignments; 

    bool executeSingleCommand(Process* proc, int coreId);
    uint32_t delaysPerExecution;
    uint32_t quantumCycles;
};
