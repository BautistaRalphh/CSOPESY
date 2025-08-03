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
#include <functional>

class Process;

enum class SchedulerAlgorithmType {
    NONE,
    fcfs,
    rr
};

struct SleepingProcess {
    std::shared_ptr<Process> process;
    long long wakeUpTime;
    int assignedCoreId;
};

class Scheduler {
public:
    Scheduler(int coreCount);
    ~Scheduler();

    void addProcess(std::shared_ptr<Process> process);
    void markCoreAvailable(int core);
    void start();
    void stop();
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

    // CPU tick statistics
    long long getTotalCpuTicks() const;
    long long getActiveCpuTicks() const;
    long long getIdleCpuTicks() const;

    void setDelaysPerExecution(uint32_t delays) { delaysPerExecution = delays; }
    uint32_t getDelaysPerExecution() const { return delaysPerExecution; }

    void setQuantumCycles(uint32_t quantum) { quantumCycles = quantum; }
    uint32_t getQuantumCycles() const { return quantumCycles; }

    using ProcessTerminationCallback = std::function<void(std::shared_ptr<Process>)>;
    void setProcessTerminationCallback(ProcessTerminationCallback callback);

    void addProcessToRRPendingQueue(std::shared_ptr<Process> process) {
        std::lock_guard<std::mutex> lock(mtx);
        rrPendingQueue.push(process);
        cv.notify_all(); 
    }

private:
    void runSchedulingLoop();

    void _runFCFSLogic(std::unique_lock<std::mutex>& lock);
    void _runRoundRobinLogic(std::unique_lock<std::mutex>& lock);

    void _markCoreAvailableUnlocked(int core);
    void _addProcessUnlocked(std::shared_ptr<Process> process);
    void _setAlgorithmTypeUnlocked(SchedulerAlgorithmType type);
    SchedulerAlgorithmType _getAlgorithmTypeUnlocked() const;
    int _getCoresUsedUnlocked() const;
    int _getCoresAvailableUnlocked() const;
    long long _getSimulatedTimeUnlocked() const;
    void _advanceSimulatedTimeUnlocked(long long deltaTime);
    void _checkSleepingProcessesUnlocked();
    bool _sleepQuickScan() const;
    bool _areAllQueuesEmptyUnlocked() const;

    std::string getCurrentTimestamp();

    int numCores;
    std::vector<bool> coreAvailable;
    std::vector<std::queue<std::shared_ptr<Process>>> processQueues;
    std::queue<std::shared_ptr<Process>> globalQueue;
    std::queue<std::shared_ptr<Process>> rrPendingQueue;
    int nextCoreForNewProcess;

    mutable std::mutex mtx;
    mutable std::condition_variable cv;
    std::unique_ptr<std::thread> schedulerThread;
    std::atomic<bool> running;
    SchedulerAlgorithmType currentAlgorithm;

    std::vector<SleepingProcess> sleepingProcesses;
    long long simulatedTime;
    std::vector<std::shared_ptr<Process>> coreAssignments;

    bool executeSingleCommand(std::shared_ptr<Process> proc, int coreId);
    uint32_t delaysPerExecution;
    uint32_t quantumCycles;
    
    mutable std::atomic<long long> totalCpuTicks;
    mutable std::atomic<long long> activeCpuTicks;
    mutable std::atomic<long long> idleCpuTicks;
    ProcessTerminationCallback onProcessTerminatedCallback = nullptr;
};
