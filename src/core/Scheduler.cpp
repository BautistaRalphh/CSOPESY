#include <queue>
#include "Scheduler.h"
#include <iostream>
#include <numeric>
#include <vector>
#include <algorithm>
#include <sstream>
#include <thread>
#include <fstream>
#include "ConsoleManager.h"
#include "memory/FlatMemoryAllocator.h"
#include <memory>

constexpr long long REAL_TIME_TICK_DURATION_MS = 50; //Lower for much faster processes

Scheduler::Scheduler(int coreCount)
    : numCores(coreCount),
      coreAvailable(coreCount, true),
      running(false),
      currentAlgorithm(SchedulerAlgorithmType::NONE),
      processQueues(coreCount),
      nextCoreForNewProcess(0),
      simulatedTime(0),
      coreAssignments(coreCount, nullptr),
      delaysPerExecution(0),
      quantumCycles(0),
      mtx(),
      cv() {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(mtx);
    _addProcessUnlocked(process);
    cv.notify_all();
}

void Scheduler::_addProcessUnlocked(std::shared_ptr<Process> process) {
    if (numCores > 0) {
        if (process->getStatus() == ProcessStatus::NEW) {
            process->setStatus(ProcessStatus::READY);
        }

        if (_getAlgorithmTypeUnlocked() == SchedulerAlgorithmType::rr) {
            globalQueue.push(process);
            process->addLogEntry("(" + getCurrentTimestamp() + ") Process " + process->getProcessName() +
                                 " (PID:" + process->getPid() + ") added to RR Global Queue.");
        } else {
            processQueues[nextCoreForNewProcess].push(process);
            process->addLogEntry("(" + getCurrentTimestamp() + ") Process " + process->getProcessName() +
                                 " (PID:" + process->getPid() + ") added to FCFS Queue " + std::to_string(nextCoreForNewProcess) + ".");
            nextCoreForNewProcess = (nextCoreForNewProcess + 1) % numCores;
        }
    } else {
        std::cerr << "[ERROR] Cannot add process: Scheduler configured with 0 cores." << std::endl;
    }
}

void Scheduler::markCoreAvailable(int core) {
    std::lock_guard<std::mutex> lock(mtx);
    _markCoreAvailableUnlocked(core);
}

void Scheduler::_markCoreAvailableUnlocked(int core) {
    if (core >= 0 && core < coreAvailable.size()) {
        coreAvailable[core] = true;
        coreAssignments[core] = nullptr;
    }
}

void Scheduler::start() {
    std::lock_guard<std::mutex> lock(mtx);
    if (_getAlgorithmTypeUnlocked() == SchedulerAlgorithmType::NONE) {
        std::cerr << "[ERROR] Scheduler algorithm not set. Cannot start." << std::endl;
        return;
    }
    if (!running.load(std::memory_order_relaxed)) {
        running.store(true, std::memory_order_release);
        schedulerThread = std::make_unique<std::thread>(&Scheduler::runSchedulingLoop, this);
    } else {
        std::cout << "[INFO] Scheduler already running. Ignoring start request." << std::endl;
    }
}

void Scheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!running.load(std::memory_order_relaxed)) return;

        running.store(false, std::memory_order_release);

        cv.notify_all();
    }

    if (schedulerThread && schedulerThread->joinable()) {
        if (schedulerThread->get_id() != std::this_thread::get_id()) {
            schedulerThread->join();
        } else {
            std::cerr << "[ERROR] Scheduler thread tried to join itself. Skipping join to avoid deadlock." << std::endl;
        }
        schedulerThread.reset();
    }
    resetCoreStates();
}

void Scheduler::resetCoreStates() {
    std::lock_guard<std::mutex> lock(mtx);
    for (size_t i = 0; i < coreAvailable.size(); ++i) {
        coreAvailable[i] = true;
    }
    std::fill(coreAssignments.begin(), coreAssignments.end(), nullptr);

    sleepingProcesses.clear();

    for (auto& queue : processQueues) { 
        while (!queue.empty()) {
            queue.pop();
        }
    }
    while (!globalQueue.empty()) { 
        globalQueue.pop();
    }
    simulatedTime = 0;
}

void Scheduler::setAlgorithmType(SchedulerAlgorithmType type) {
    std::lock_guard<std::mutex> lock(mtx);
    _setAlgorithmTypeUnlocked(type);
}

void Scheduler::_setAlgorithmTypeUnlocked(SchedulerAlgorithmType type) {
    currentAlgorithm = type;
}

SchedulerAlgorithmType Scheduler::getAlgorithmType() const {
    std::lock_guard<std::mutex> lock(mtx);
    return _getAlgorithmTypeUnlocked();
}

SchedulerAlgorithmType Scheduler::_getAlgorithmTypeUnlocked() const {
    return currentAlgorithm;
}

int Scheduler::getTotalCores() const {
    return numCores;
}

int Scheduler::getCoresUsed() const {
    std::lock_guard<std::mutex> lock(mtx);
    return _getCoresUsedUnlocked();
}

int Scheduler::_getCoresUsedUnlocked() const {
    int usedCount = 0;
    for (bool available : coreAvailable) {
        if (!available) {
            usedCount++;
        }
    }
    return usedCount;
}

int Scheduler::getCoresAvailable() const {
    std::lock_guard<std::mutex> lock(mtx);
    return _getCoresAvailableUnlocked();
}

int Scheduler::_getCoresAvailableUnlocked() const {
    int availableCount = 0;
    for (bool available : coreAvailable) {
        if (available) {
            availableCount++;
        }
    }
    return availableCount;
}

double Scheduler::getCpuUtilization() const {
    int total = getTotalCores();
    if (total == 0) {
        return 0.0;
    }
    int used = getCoresUsed();
    return static_cast<double>(used) / total * 100.0;
}

bool Scheduler::isRunning() const {
    return running.load(std::memory_order_relaxed);
}

std::string Scheduler::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::tm localTime;
    localtime_s(&localTime, &now_c);

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S%p", &localTime);
    return std::string(buffer);
}

long long Scheduler::getSimulatedTime() const {
    std::lock_guard<std::mutex> lock(mtx);
    return _getSimulatedTimeUnlocked();
}

long long Scheduler::_getSimulatedTimeUnlocked() const {
    return simulatedTime;
}

void Scheduler::advanceSimulatedTime(long long deltaTime) {
    std::lock_guard<std::mutex> lock(mtx);
    _advanceSimulatedTimeUnlocked(deltaTime);
}

void Scheduler::_advanceSimulatedTimeUnlocked(long long deltaTime) {
    if (deltaTime > 0) {
        simulatedTime += deltaTime;
    }
}

void Scheduler::checkSleepingProcesses() {
    std::lock_guard<std::mutex> lock(mtx);
    _checkSleepingProcessesUnlocked();
}

void Scheduler::_checkSleepingProcessesUnlocked() {
    std::vector<std::shared_ptr<Process>> wokenProcesses;
    std::vector<SleepingProcess> stillSleeping;

    for (const auto& sleepCtx : sleepingProcesses) {
        if (_getSimulatedTimeUnlocked() >= sleepCtx.wakeUpTime) {
            sleepCtx.process->addLogEntry(
                "(" + getCurrentTimestamp() + ") Core:" + std::to_string(sleepCtx.assignedCoreId) +
                " Process " + sleepCtx.process->getProcessName() + 
                " (PID:" + sleepCtx.process->getPid() + 
                ") woken up at simulated time " + std::to_string(_getSimulatedTimeUnlocked()) + ".");

            sleepCtx.process->setStatus(ProcessStatus::READY);
            wokenProcesses.push_back(sleepCtx.process);
        } else {
            stillSleeping.push_back(sleepCtx);
        }
    }

    sleepingProcesses = std::move(stillSleeping);

    for (const auto& proc : wokenProcesses) {
        _addProcessUnlocked(proc);
    }

    if (!wokenProcesses.empty()) {
        cv.notify_all();
    }
}

bool Scheduler::_sleepQuickScan() const {
    for (const auto& sleepCtx : sleepingProcesses) {
        if (simulatedTime >= sleepCtx.wakeUpTime) {
            return true;
        }
    }
    return false;
}

bool Scheduler::executeSingleCommand(std::shared_ptr<Process> proc, int coreId) {
    if (!running.load(std::memory_order_relaxed)) return false;

    const ParsedCommand* cmd = proc->getNextCommand(); 
    bool commandExecuted = false;

    if (!cmd) {
        proc->setStatus(ProcessStatus::TERMINATED);
        proc->setFinishTime(getCurrentTimestamp());

        // Free memory when process terminates
        auto memoryAllocator = ConsoleManager::getInstance()->getMemoryAllocator();
        if (memoryAllocator) {
            memoryAllocator->deallocate(proc);
        }

        if (!proc->isLoopStackEmpty()) {
            proc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(coreId) + 
                              " Process " + proc->getProcessName() + " (PID:" + proc->getPid() + ") TERMINATED (after loop).");
        } else {
            proc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(coreId) + 
                              " Process " + proc->getProcessName() + " (PID:" + proc->getPid() + ") TERMINATED.");
        }

        if (onProcessTerminatedCallback) {
            onProcessTerminatedCallback(proc);
        }

        return false;
    }

    std::stringstream log;
    log << "(" + getCurrentTimestamp() + ") Core:" + std::to_string(coreId) + " ";
    std::string commandToLog = "";

    switch (cmd->type) {
        case CommandType::PRINT: {
            if (!cmd->args.empty()) {
                commandToLog = "PRINT " + cmd->args[0];
                proc->addLogEntry(log.str() + commandToLog);
            }
            commandExecuted = true;
            break;
        }
        case CommandType::DECLARE: {
            if (cmd->args.size() == 2) {
                const std::string& var = cmd->args[0];
                uint16_t val = static_cast<uint16_t>(std::stoul(cmd->args[1]));
                proc->declareVariable(var, val);
                commandToLog = "DECLARE " + var + " = " + std::to_string(val);
                proc->addLogEntry(log.str() + commandToLog);
            }
            commandExecuted = true;
            break;
        }
        case CommandType::ADD: {
            if (cmd->args.size() == 3) {
                const std::string& dest = cmd->args[0];
                const std::string& src1 = cmd->args[1];
                const std::string& src2 = cmd->args[2];

                uint16_t val1 = 0, val2 = 0;

                auto getOrDeclare = [&](const std::string& name, uint16_t& value_ref) {
                    if (proc->doesVariableExist(name)) {
                        proc->getVariableValue(name, value_ref);
                    } else if (name.find_first_not_of("0123456789") == std::string::npos) {
                        value_ref = static_cast<uint16_t>(std::stoul(name));
                    } else {
                        proc->declareVariable(name, 0);
                        value_ref = 0;
                    }
                };

                getOrDeclare(src1, val1);
                getOrDeclare(src2, val2);

                uint16_t result = val1 + val2;
                proc->setVariableValue(dest, result);
                commandToLog = "ADD " + dest + " = " + src1 + "(" + std::to_string(val1) + ") + " + src2 + "(" + std::to_string(val2) + ") => " + dest + "(" + std::to_string(result) + ")";
                proc->addLogEntry(log.str() + commandToLog);
            }
            commandExecuted = true;
            break;
        }
        case CommandType::SUBTRACT: {
            if (cmd->args.size() == 3) {
                const std::string& dest = cmd->args[0];
                const std::string& src1 = cmd->args[1];
                const std::string& src2 = cmd->args[2];

                uint16_t val1 = 0, val2 = 0;

                auto getOrDeclare = [&](const std::string& name, uint16_t& value_ref) {
                    if (proc->doesVariableExist(name)) {
                        proc->getVariableValue(name, value_ref);
                    } else if (name.find_first_not_of("0123456789") == std::string::npos) {
                        value_ref = static_cast<uint16_t>(std::stoul(name));
                    } else {
                        proc->declareVariable(name, 0);
                        value_ref = 0;
                    }
                };

                getOrDeclare(src1, val1);
                getOrDeclare(src2, val2);

                uint16_t result = val1 - val2;
                proc->setVariableValue(dest, result);
                commandToLog = "SUBTRACT " + dest + " = " + src1 + "(" + std::to_string(val1) + ") - " + src2 + "(" + std::to_string(val2) + ") => " + dest + "(" + std::to_string(result) + ")";
                proc->addLogEntry(log.str() + commandToLog);
            }
            commandExecuted = true;
            break;
        }
        case CommandType::SLEEP: {
            if (!cmd->args.empty()) {
                int ticks = std::stoi(cmd->args[0]);
                commandToLog = "SLEEP for " + std::to_string(ticks) + " ticks.";
                proc->addLogEntry(log.str() + commandToLog);

                proc->setStatus(ProcessStatus::PAUSED);
                sleepingProcesses.push_back({proc, simulatedTime + ticks, coreId});

                proc->setCpuCoreExecuting(-1);
                _markCoreAvailableUnlocked(coreId);
                return false; 
            }
            commandExecuted = true;
            break;
        }
        case CommandType::FOR:
        case CommandType::END_FOR:
            commandToLog = (cmd->type == CommandType::FOR ? "FOR loop entered/re-entered" : "END_FOR reached (loop control)");
            proc->addLogEntry(log.str() + commandToLog);
            commandExecuted = true;
            break;
        default:
            commandToLog = "Unknown or unhandled command type.";
            proc->addLogEntry(log.str() + commandToLog);
            commandExecuted = true;
            break;
    }

    return commandExecuted; 
}

bool Scheduler::_areAllQueuesEmptyUnlocked() const {
    if (_getAlgorithmTypeUnlocked() == SchedulerAlgorithmType::rr) {
        return globalQueue.empty();
    } else {
        for (const auto& q : processQueues) {
            if (!q.empty()) return false;
        }
        return true;
    }
}

void Scheduler::runSchedulingLoop() {
    std::cout << "[Scheduler] Scheduling loop started.\n";
    auto lastRealTimeTick = std::chrono::high_resolution_clock::now();

    while (running.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lock(mtx);

        auto now = std::chrono::high_resolution_clock::now();
        long long actualDeltaTimeMillis = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRealTimeTick).count();

        if (actualDeltaTimeMillis >= REAL_TIME_TICK_DURATION_MS) {
            lastRealTimeTick = now;
            _advanceSimulatedTimeUnlocked(actualDeltaTimeMillis / REAL_TIME_TICK_DURATION_MS);
            _checkSleepingProcessesUnlocked();
        }

        bool hasReadyProcessesInQueue = (_getAlgorithmTypeUnlocked() == SchedulerAlgorithmType::rr)
            ? !globalQueue.empty()
            : std::any_of(processQueues.begin(), processQueues.end(), [](const auto& q) { return !q.empty(); });

        bool anyCoreRunning = (_getCoresUsedUnlocked() > 0);
        bool hasSleepers = _sleepQuickScan();

        if (!hasReadyProcessesInQueue && !anyCoreRunning && !hasSleepers) {
            cv.wait_for(lock, std::chrono::milliseconds(REAL_TIME_TICK_DURATION_MS), [&] {
                return !running.load(std::memory_order_relaxed)
                    || !_areAllQueuesEmptyUnlocked()
                    || _sleepQuickScan();
            });
        } else {
            cv.wait_for(lock, std::chrono::microseconds(100), [&] {
                return !running.load(std::memory_order_relaxed);
            });
        }

        if (!running.load(std::memory_order_relaxed)) break;

        // Always retry allocation for pending RR processes every quantum, even if all processes are sleeping
        if (_getAlgorithmTypeUnlocked() == SchedulerAlgorithmType::rr) {
            size_t pendingCount = rrPendingQueue.size();
            for (size_t p = 0; p < pendingCount; ++p) {
                auto proc = rrPendingQueue.front();
                rrPendingQueue.pop();
                auto memoryAllocator = ConsoleManager::getInstance()->getMemoryAllocator();
                bool allocated = false;
                if (memoryAllocator) {
                    allocated = (memoryAllocator->allocate(proc) != nullptr);
                }
                if (allocated) {
                    globalQueue.push(proc);
                    proc->addLogEntry("(" + getCurrentTimestamp() + ") Process " + proc->getProcessName() +
                                     " (PID:" + proc->getPid() + ") memory allocated, added to RR Global Queue.");
                } else {
                    rrPendingQueue.push(proc);
                }
            }
        }
        switch (_getAlgorithmTypeUnlocked()) {
            case SchedulerAlgorithmType::fcfs:
                _runFCFSLogic(lock);
                break;
            case SchedulerAlgorithmType::rr:
                _runRoundRobinLogic(lock);
                break;
            case SchedulerAlgorithmType::NONE:
                std::cerr << "[WARNING] Scheduler running with no algorithm selected.\n";
                break;
        }

        lock.unlock();
    }

    std::cout << "[Scheduler] Exiting scheduling loop.\n";
}

void Scheduler::_runFCFSLogic(std::unique_lock<std::mutex>& lock) {
    for (int i = 0; i < numCores; ++i) {
        if (!running.load(std::memory_order_relaxed)) return;

        bool coreFreed = false;
        if (coreAssignments[i]) {
            std::shared_ptr<Process> proc = coreAssignments[i];

            if (proc->getStatus() == ProcessStatus::RUNNING) {
                bool hasMoreCommands = executeSingleCommand(proc, i);

                if (!running.load(std::memory_order_relaxed)) return;

                if (hasMoreCommands) {
                    _advanceSimulatedTimeUnlocked(1 + delaysPerExecution);
                } else if (proc->getStatus() == ProcessStatus::TERMINATED) {
                    _advanceSimulatedTimeUnlocked(1 + delaysPerExecution);
                    proc->setCpuCoreExecuting(-1);
                    _markCoreAvailableUnlocked(i);
                    coreFreed = true;
                }
            } else if (proc->getStatus() == ProcessStatus::PAUSED || proc->getStatus() == ProcessStatus::TERMINATED) {
                if (proc->getCpuCoreExecuting() == i) {
                    proc->setCpuCoreExecuting(-1);
                }
                _markCoreAvailableUnlocked(i);
                coreFreed = true;
            }
        } else {
            coreFreed = coreAvailable[i];
        }

        // Always try to dispatch a process to any available core immediately after freeing
        if (coreAvailable[i] && coreFreed) {
            std::shared_ptr<Process> nextProc = nullptr;
            int selectedQueueIdx = -1;
            for (int q_idx = 0; q_idx < numCores; ++q_idx) {
                if (!processQueues[q_idx].empty()) {
                    nextProc = processQueues[q_idx].front();
                    selectedQueueIdx = q_idx;
                    break;
                }
            }
            if (nextProc && selectedQueueIdx != -1) {
                processQueues[selectedQueueIdx].pop();
                nextProc->setCpuCoreExecuting(i);
                nextProc->setStatus(ProcessStatus::RUNNING);
                coreAvailable[i] = false;
                coreAssignments[i] = nextProc;
                nextProc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(i) +
                    " Process " + nextProc->getProcessName() + " (PID:" + nextProc->getPid() + ") dispatched.");
            }
        }
    }
}

void Scheduler::_runRoundRobinLogic(std::unique_lock<std::mutex>& lock) {
    const int effectiveQuantum = (quantumCycles > 0) ? quantumCycles : 3;

    auto logMemorySnapshot = [&](int quantumValue) {
        // No-op: removed per-process .txt report generation
    };

    size_t pendingCount = rrPendingQueue.size();
    for (size_t p = 0; p < pendingCount; ++p) {
        auto proc = rrPendingQueue.front();
        rrPendingQueue.pop();
        auto memoryAllocator = ConsoleManager::getInstance()->getMemoryAllocator();
        bool allocated = false;

        if (memoryAllocator) {
            allocated = (memoryAllocator->allocate(proc) != nullptr);
        }

        if (allocated) {
            globalQueue.push(proc);
            proc->addLogEntry("(" + getCurrentTimestamp() + ") Process " + proc->getProcessName() +
                             " (PID:" + proc->getPid() + ") memory allocated, added to RR Global Queue.");
        } else {
            rrPendingQueue.push(proc);
            proc->addLogEntry("(" + getCurrentTimestamp() + ") Process " + proc->getProcessName() +
                             " (PID:" + proc->getPid() + ") failed memory allocation. Remaining in pending queue.");
        }
    }

    for (int i = 0; i < numCores; ++i) {
        if (coreAvailable[i] && !globalQueue.empty()) {
            auto nextProc = globalQueue.front();
            globalQueue.pop();
            nextProc->setCpuCoreExecuting(i);
            nextProc->setStatus(ProcessStatus::RUNNING);
            coreAvailable[i] = false;
            coreAssignments[i] = nextProc;
            nextProc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(i) +
                " Process " + nextProc->getProcessName() + " (PID:" + nextProc->getPid() + ") dispatched.");
        }
    }

    for (int i = 0; i < numCores; ++i) {
        if (!running.load(std::memory_order_relaxed)) return;

        bool coreFreed = false;
        std::shared_ptr<Process> proc = coreAssignments[i];
        if (proc) {
            if (proc->getStatus() == ProcessStatus::RUNNING && proc->getCpuCoreExecuting() == i) {
                int executedCommandsInSlice = 0;
                while (executedCommandsInSlice < effectiveQuantum) {
                    if (!running.load(std::memory_order_relaxed)) return;
                    bool commandStillRunning = executeSingleCommand(proc, i);

                    if (!commandStillRunning || proc->getStatus() == ProcessStatus::PAUSED || proc->getStatus() == ProcessStatus::TERMINATED)
                        break;
                    ++executedCommandsInSlice;
                    _advanceSimulatedTimeUnlocked(1 + delaysPerExecution);
                }

                logMemorySnapshot(effectiveQuantum);

                if (!running.load(std::memory_order_relaxed)) return;
                if (proc->getStatus() == ProcessStatus::RUNNING) {
                    proc->setStatus(ProcessStatus::READY);
                    proc->setCpuCoreExecuting(-1);
                    globalQueue.push(proc);
                    proc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(i) +
                        " Process " + proc->getProcessName() + " (PID:" + proc->getPid() + ") preempted, added to RR Global Queue.");
                    _markCoreAvailableUnlocked(i);
                    coreFreed = true;
                } else {
                    _markCoreAvailableUnlocked(i);
                    coreFreed = true;
                }
            } else if (proc->getStatus() == ProcessStatus::PAUSED || proc->getStatus() == ProcessStatus::TERMINATED) {
                if (proc->getCpuCoreExecuting() == i || proc->getCpuCoreExecuting() == -1) {
                    _markCoreAvailableUnlocked(i);
                    coreFreed = true;
                }
            }
        } else {
            coreFreed = coreAvailable[i];
        }
        if (coreAvailable[i] && coreFreed) {
            std::shared_ptr<Process> nextProc = nullptr;
            if (!globalQueue.empty()) {
                nextProc = globalQueue.front();
                globalQueue.pop();
                nextProc->setCpuCoreExecuting(i);
                nextProc->setStatus(ProcessStatus::RUNNING);
                coreAvailable[i] = false;
                coreAssignments[i] = nextProc;
                nextProc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(i) +
                    " Process " + nextProc->getProcessName() + " (PID:" + nextProc->getPid() + ") dispatched.");
            }
        }
    }
}

void Scheduler::setProcessTerminationCallback(ProcessTerminationCallback callback) {
    std::lock_guard<std::mutex> lock(mtx);
    onProcessTerminatedCallback = callback;
}
