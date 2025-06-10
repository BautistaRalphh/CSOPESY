#include "Scheduler.h"
#include <iostream>

Scheduler::Scheduler(int coreCount)
    : coreAvailable(coreCount, true), running(false) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::addProcess(const Process& process) {
    std::lock_guard<std::mutex> lock(mtx);
    processQueue.push(process);
    cv.notify_all();
}

void Scheduler::markCoreAvailable(int coreId) {
    std::lock_guard<std::mutex> lock(mtx);
    coreAvailable[coreId] = true;
    cv.notify_all();
}

void Scheduler::start() {
    running = true;
    schedulerThread = std::thread(&Scheduler::schedulerLoop, this);
}

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

void Scheduler::schedulerLoop() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !running || !processQueue.empty(); });

        if (!running && processQueue.empty())
            break;

        for (int i = 0; i < coreAvailable.size(); ++i) {
            if (coreAvailable[i] && !processQueue.empty()) {
                Process proc = processQueue.front();
                processQueue.pop();
                coreAvailable[i] = false;

                // Simulated assignment
                std::cout << "[Scheduler] Assigning Process " << proc.getPid()
                          << " to Core " << i << std::endl;

            }
        }
    }
}
