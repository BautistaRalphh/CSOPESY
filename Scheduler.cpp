#include "Scheduler.h"
#include <iostream>

// Init core states and flags
Scheduler::Scheduler(int coreCount)
    : coreAvailable(coreCount, true), running(false) {}

// Stop scheduler on destruction
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
    coreAvailable[core] = true;
    cv.notify_all();
}

// Launch scheduler thread
void Scheduler::start() {
    running = true;
    schedulerThread = std::thread(&Scheduler::FCFS_Loop, this);
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

// FCFS logic: assign queued process to free core
void Scheduler::FCFS_Loop() {
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
