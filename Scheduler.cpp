#include "Scheduler.h"
#include <iostream>
#include <iostream>  
#include <numeric>

// Init core states and flags
Scheduler::Scheduler(int coreCount) : coreAvailable(coreCount, true), running(false), currentAlgorithm(SchedulerAlgorithmType::NONE) {}

Scheduler::~Scheduler() {
    stop();
}

// Queue a new process
void Scheduler::addProcess(Process* process) {
    std::lock_guard<std::mutex> lock(mtx);
    processQueue.push(process); 
    cv.notify_all();
}

// Free a core
void Scheduler::markCoreAvailable(int core) {
    std::lock_guard<std::mutex> lock(mtx);
    if (core >= 0 && core < coreAvailable.size()) {
        coreAvailable[core] = true;
    }
    cv.notify_all();
}

// Launch scheduler thread
void Scheduler::start() {
    if (currentAlgorithm == SchedulerAlgorithmType::NONE) {
        std::cerr << "[ERROR] Scheduler algorithm not set. Cannot start." << std::endl;
        return;
    }
    if (!running) {
        running = true;
        schedulerThread = std::thread(&Scheduler::runSchedulingLoop, this);
    }
}

// Stop scheduler thread
void Scheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();
    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }
}

void Scheduler::resetCoreStates() {
    std::lock_guard<std::mutex> lock(mtx);
    for (size_t i = 0; i < coreAvailable.size(); ++i) {
        coreAvailable[i] = true;
    }
}

void Scheduler::setAlgorithmType(SchedulerAlgorithmType type) {
    std::lock_guard<std::mutex> lock(mtx);
    currentAlgorithm = type;
}

// Returns the total number of CPU cores the scheduler manages
int Scheduler::getTotalCores() const {
    return coreAvailable.size();
}

// Returns the number of cores currently marked as busy (not available)
int Scheduler::getCoresUsed() const {
    std::lock_guard<std::mutex> lock(mtx);
    int usedCount = 0;
    for (bool available : coreAvailable) {
        if (!available) {
            usedCount++;
        }
    }
    return usedCount;
}

// Returns the number of cores currently marked as available
int Scheduler::getCoresAvailable() const {
    std::lock_guard<std::mutex> lock(mtx); 
    int availableCount = 0;
    for (bool available : coreAvailable) {
        if (available) { 
            availableCount++;
        }
    }
    return availableCount;
}

// Calculates and returns the CPU utilization as a percentage
double Scheduler::getCpuUtilization() const {
    int total = getTotalCores();
    if (total == 0) {
        return 0.0; 
    }
    int used = getCoresUsed();
    return static_cast<double>(used) / total * 100.0;
}

bool Scheduler::isRunning() const {
    return running.load();
}

void Scheduler::runSchedulingLoop() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { 
            return !running || !processQueue.empty(); 
        });

        if (!running && processQueue.empty()) { 
            break; 
        }

        switch (currentAlgorithm) {
            case SchedulerAlgorithmType::FCFS:
                _runFCFSLogic(lock);
                break;
            case SchedulerAlgorithmType::NONE:
                std::cerr << "[WARNING] Scheduler loop running with no algorithm selected." << std::endl;
                break;
        }
    }
}

void Scheduler::_runFCFSLogic(std::unique_lock<std::mutex>& lock) {
    for (int i = 0; i < coreAvailable.size(); ++i) {
        if (coreAvailable[i] && !processQueue.empty()) {
            Process* proc = processQueue.front();
            processQueue.pop();

            proc->setCpuCoreExecuting(i);
            proc->setStatus(ProcessStatus::RUNNING);
            coreAvailable[i] = false;

            std::thread([this, proc, i]() {
                int cmdIndex = 0;

                while (const ParsedCommand* cmd = proc->getNextCommand()) {
                    while (proc->getStatus() == ProcessStatus::PAUSED) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }

                    proc->setStatus(ProcessStatus::RUNNING);
                    proc->setCurrentCommandIndex(cmdIndex);

                    // std::cout << "DEBUG: FCFS assigning " << proc->getProcessName() << " to core " << i << std::endl;

                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    ++cmdIndex;
                }

                proc->setStatus(ProcessStatus::FINISHED);
                markCoreAvailable(i);
            }).detach();
        }
    }
}