#include "FCFS_Scheduler.h"
#include <iostream>

// Init core states and flags
FCFS_Scheduler::FCFS_Scheduler(int coreCount)
    : coreAvailable(coreCount, true), running(false) {}

// Stop scheduler on destruction
FCFS_Scheduler::~FCFS_Scheduler() {
    stop();
}

// Queue a new process
void FCFS_Scheduler::addProcess(const Process& process) {
    std::lock_guard<std::mutex> lock(mtx);
    processQueue.push(const_cast<Process*>(&process));
    cv.notify_all();
}

// Free a core
void FCFS_Scheduler::markCoreAvailable(int core) {
    std::lock_guard<std::mutex> lock(mtx);
    coreAvailable[core] = true;
    cv.notify_all();
}

// Launch scheduler thread
void FCFS_Scheduler::start() {
    running = true;
    schedulerThread = std::thread(&FCFS_Scheduler::schedulerLoop, this);
}

// Stop scheduler thread
void FCFS_Scheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();
    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }
}

// FCFS logic: assign queued process to free core
void FCFS_Scheduler::schedulerLoop() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !running || !processQueue.empty(); });

        if (!running && processQueue.empty())
            break;

        for (int i = 0; i < coreAvailable.size(); ++i) {
            if (coreAvailable[i] && !processQueue.empty()) {
                Process* proc = processQueue.front();
                processQueue.pop();

                proc->setCpuCoreExecuting(i);
                proc->setStatus(ProcessStatus::RUNNING);
                coreAvailable[i] = false;

                std::cout << "[Scheduler] Assigning Process " << proc->getPid()
                          << " to Core " << i << std::endl;

            }
        }
    }
}
