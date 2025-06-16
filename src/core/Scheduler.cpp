#include "Scheduler.h"

#include <iostream>
#include <iostream>  
#include <numeric>

Scheduler::Scheduler(int coreCount)
    : numCores(coreCount), 
      coreAvailable(coreCount, true),
      running(false),
      currentAlgorithm(SchedulerAlgorithmType::NONE),
      processQueues(coreCount),
      nextCoreForNewProcess(0) 
{}

Scheduler::~Scheduler() {
    stop();
}

// Queue a new process
void Scheduler::addProcess(Process* process) {
    std::lock_guard<std::mutex> lock(mtx);
    if (numCores > 0) {
        processQueues[nextCoreForNewProcess].push(process); 
        nextCoreForNewProcess = (nextCoreForNewProcess + 1) % numCores;
    } else {
        std::cerr << "[ERROR] Cannot add process: Scheduler configured with 0 cores." << std::endl;
    }
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

SchedulerAlgorithmType Scheduler::getAlgorithmType() const {
    std::lock_guard<std::mutex> lock(mtx); 
    return currentAlgorithm;
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
    while (running.load()) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] {
            if (!running.load()) return true; 
            for (int i = 0; i < numCores; ++i) {
                if (coreAvailable[i] && !processQueues[i].empty()) { 
                    return true;
                }
            }
            return false; 
        });

        if (!running.load() && getCoresUsed() == getTotalCores()) { 
            bool allQueuesEmpty = true;
            for(int i = 0; i < numCores; ++i) {
                if (!processQueues[i].empty()) {
                    allQueuesEmpty = false;
                    break;
                }
            }
            if(allQueuesEmpty) break;
        }

        switch (currentAlgorithm) {
            case SchedulerAlgorithmType::FCFS:
                _runFCFSLogic(lock);
                break;
            case SchedulerAlgorithmType::NONE:
                std::cerr << "[WARNING] Scheduler loop running with no algorithm selected." << std::endl;
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                break;
        }
    }
}

std::string Scheduler::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&now_c);

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S%p", &localTime);
    return std::string(buffer);
}

void Scheduler::_runFCFSLogic(std::unique_lock<std::mutex>& lock) {
    for (int i = 0; i < numCores; ++i) { 
        if (coreAvailable[i] && !processQueues[i].empty()) { 
            Process* proc = processQueues[i].front(); 
            processQueues[i].pop(); 

            proc->setCpuCoreExecuting(i);
            proc->setStatus(ProcessStatus::RUNNING);

            coreAvailable[i] = false; 

            std::thread([this, proc, i]() {
                int cmdIndex = 0;

                while (const ParsedCommand* cmd = proc->getNextCommand()) {
                    proc->setCurrentCommandIndex(cmdIndex);

                    if (cmd->type == CommandType::PRINT && !cmd->args.empty()) {
                        std::stringstream log;
                        log << "(" << getCurrentTimestamp() << ") Core:" << i << " \"" << cmd->args[0] << "\"";
                        proc->addLogEntry(log.str());
                    }

                    ++cmdIndex;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
                }

                proc->setStatus(ProcessStatus::FINISHED);
                proc->setFinishTime(getCurrentTimestamp());

                proc->writeToTextFile(); // Will be removed after hw6

                markCoreAvailable(i); 
            }).detach();

        }
    }
}
