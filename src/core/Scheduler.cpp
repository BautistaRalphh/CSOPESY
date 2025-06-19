#include "Scheduler.h"

#include <iostream>
#include <numeric>
#include <vector>
#include <algorithm>
#include <sstream>
#include <thread>

constexpr long long REAL_TIME_TICK_DURATION_MS = 50;

Scheduler::Scheduler(int coreCount)
    : numCores(coreCount),
      coreAvailable(coreCount, true),
      running(false),
      currentAlgorithm(SchedulerAlgorithmType::NONE),
      processQueues(coreCount),
      nextCoreForNewProcess(0),
      simulatedTime(0),
      coreAssignments(coreCount, nullptr),
      delaysPerExecution(0) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::addProcess(Process* process) {
    std::lock_guard<std::mutex> lock(mtx);
    _addProcessUnlocked(process);
    cv.notify_all();
}

void Scheduler::_addProcessUnlocked(Process* process) {
    if (numCores > 0) {
        if (process->getStatus() == ProcessStatus::NEW) {
            process->setStatus(ProcessStatus::READY);
        }
        processQueues[nextCoreForNewProcess].push(process);
        nextCoreForNewProcess = (nextCoreForNewProcess + 1) % numCores;
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
    if (!running) {
        running = true;
        schedulerThread = std::thread(&Scheduler::runSchedulingLoop, this);
    }
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
    return running.load();
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
    std::vector<Process*> wokenProcesses;

    std::vector<SleepingProcess> stillSleeping;
    for (const auto& sleepCtx : sleepingProcesses) {
        if (_getSimulatedTimeUnlocked() >= sleepCtx.wakeUpTime) {
            sleepCtx.process->addLogEntry(
                "(" + getCurrentTimestamp() + ") Core:" + std::to_string(sleepCtx.assignedCoreId) +
                " Process " + sleepCtx.process->getProcessName() + " (PID:" + sleepCtx.process->getPid() + ") woken up at simulated time " + std::to_string(_getSimulatedTimeUnlocked()) + ".");

            sleepCtx.process->setStatus(ProcessStatus::READY);
            wokenProcesses.push_back(sleepCtx.process);
        } else {
            stillSleeping.push_back(sleepCtx);
        }
    }
    sleepingProcesses = std::move(stillSleeping);

    for (Process* proc : wokenProcesses) {
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

bool Scheduler::executeSingleCommand(Process* proc, int coreId) {
    const ParsedCommand* cmd = nullptr;
    bool commandExecuted = false;

    while (true) {
        cmd = proc->getNextCommand();

        if (!cmd) {
            if (!proc->isLoopStackEmpty()) {
                proc->setStatus(ProcessStatus::FINISHED);
                proc->setFinishTime(getCurrentTimestamp());
                proc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(coreId) + " Process " + proc->getProcessName() + " (PID:" + proc->getPid() + ") FINISHED (after loop).");
            } else {
                proc->setStatus(ProcessStatus::FINISHED);
                proc->setFinishTime(getCurrentTimestamp());
                proc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(coreId) + " Process " + proc->getProcessName() + " (PID:" + proc->getPid() + ") FINISHED.");
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
                continue;
            default:
                commandToLog = "Unknown or unhandled command type.";
                proc->addLogEntry(log.str() + commandToLog);
                commandExecuted = true;
                break;
        }
        break;
    }

    return commandExecuted;
}

void Scheduler::runSchedulingLoop() {
    auto lastRealTimeTick = std::chrono::high_resolution_clock::now();

    while (running.load()) {
        std::unique_lock<std::mutex> lock(mtx);

        auto now = std::chrono::high_resolution_clock::now();
        long long actualDeltaTimeMillis = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRealTimeTick).count();
        lastRealTimeTick = now;

        long long simulatedTicksToAdvance = actualDeltaTimeMillis / REAL_TIME_TICK_DURATION_MS;
        if (simulatedTicksToAdvance > 0) {
            _advanceSimulatedTimeUnlocked(simulatedTicksToAdvance);
        }
        
        _checkSleepingProcessesUnlocked();

        bool hasReadyProcessesInQueue = false;
        for (int i = 0; i < numCores; ++i) {
            if (!processQueues[i].empty()) {
                hasReadyProcessesInQueue = true;
                break;
            }
        }
        bool anyCoreRunning = (_getCoresUsedUnlocked() > 0);

        if (!hasReadyProcessesInQueue && !anyCoreRunning && !_sleepQuickScan()) {
             cv.wait_for(lock, std::chrono::milliseconds(REAL_TIME_TICK_DURATION_MS), [&]{
                return !running.load() || hasReadyProcessesInQueue || _sleepQuickScan();
             });
        } else {
             cv.wait_for(lock, std::chrono::microseconds(1), [&]{
                 return !running.load() || hasReadyProcessesInQueue || _sleepQuickScan();
             });
        }

        if (!running.load()) {
            bool allQueuesEmpty = true;
            for(int i = 0; i < numCores; ++i) {
                if (!processQueues[i].empty() || coreAssignments[i] != nullptr) {
                    allQueuesEmpty = false;
                    break;
                }
            }
            if(allQueuesEmpty && sleepingProcesses.empty()) {
                break;
            }
        }

        switch (_getAlgorithmTypeUnlocked()) {
            case SchedulerAlgorithmType::FCFS:
                _runFCFSLogic(lock);
                break;
            case SchedulerAlgorithmType::RR:
                _runRoundRobinLogic(lock);
                break;
            case SchedulerAlgorithmType::NONE:
                std::cerr << "[WARNING] Scheduler loop running with no algorithm selected." << std::endl;
                break;
        }
    }
    std::cout << "Scheduler thread stopping." << std::endl;
}

void Scheduler::_runFCFSLogic(std::unique_lock<std::mutex>& lock) {
    for (int i = 0; i < numCores; ++i) {
        if (coreAssignments[i] != nullptr) {
            Process* proc = coreAssignments[i];

            if (proc->getStatus() == ProcessStatus::RUNNING) {
                bool hasMoreCommands = executeSingleCommand(proc, i);

                if (hasMoreCommands) {
                    _advanceSimulatedTimeUnlocked(1 + delaysPerExecution);
                } else if (proc->getStatus() == ProcessStatus::FINISHED) {
                    _advanceSimulatedTimeUnlocked(1 + delaysPerExecution);
                    proc->setCpuCoreExecuting(-1);
                    _markCoreAvailableUnlocked(i);
                }
            } else if (proc->getStatus() == ProcessStatus::PAUSED || proc->getStatus() == ProcessStatus::FINISHED) {
                if (proc->getCpuCoreExecuting() == i) {
                    proc->setCpuCoreExecuting(-1);
                }
                _markCoreAvailableUnlocked(i);
            }
        }
    }

    for (int i = 0; i < numCores; ++i) {
        if (coreAvailable[i]) {
            Process* nextProc = nullptr;
            int selectedQueueIdx = -1;

            for(int q_idx = 0; q_idx < numCores; ++q_idx) {
                if (!processQueues[q_idx].empty()) {
                    nextProc = processQueues[q_idx].front();
                    selectedQueueIdx = q_idx;
                    break;
                }
            }

            if (nextProc != nullptr && selectedQueueIdx != -1) {
                processQueues[selectedQueueIdx].pop();
                
                nextProc->setCpuCoreExecuting(i);
                nextProc->setStatus(ProcessStatus::RUNNING);
                coreAvailable[i] = false;
                coreAssignments[i] = nextProc;

                nextProc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(i) + " Process " + nextProc->getProcessName() + " (PID:" + nextProc->getPid() + ") dispatched.");

                bool hasMoreCommands = executeSingleCommand(nextProc, i);
                
                if (hasMoreCommands) {
                    _advanceSimulatedTimeUnlocked(1 + delaysPerExecution);
                } else if (nextProc->getStatus() == ProcessStatus::FINISHED) {
                    _advanceSimulatedTimeUnlocked(1 + delaysPerExecution);
                    _markCoreAvailableUnlocked(i);
                }
            }
        }
    }
}

void Scheduler::_runRoundRobinLogic(std::unique_lock<std::mutex>& lock) {
    const int timeSlice = 3;

    for (int i = 0; i < numCores; ++i) {
        if (coreAssignments[i] != nullptr) {
            Process* proc = coreAssignments[i];

            if (proc->getStatus() == ProcessStatus::RUNNING) {
                int executed = 0;
                while (executed < timeSlice) {
                    if (!executeSingleCommand(proc, i)) break;
                    ++executed;
                    _advanceSimulatedTimeUnlocked(1 + delaysPerExecution);
                }

                if (proc->getStatus() == ProcessStatus::RUNNING) {
                    proc->setStatus(ProcessStatus::READY);
                    proc->setCpuCoreExecuting(-1);
                    processQueues[i].push(proc);
                }

                _markCoreAvailableUnlocked(i);
            }
        }
    }

    for (int i = 0; i < numCores; ++i) {
        if (coreAvailable[i] && !processQueues[i].empty()) {
            Process* proc = processQueues[i].front();
            processQueues[i].pop();

            proc->setStatus(ProcessStatus::RUNNING);
            proc->setCpuCoreExecuting(i);
            coreAvailable[i] = false;
            coreAssignments[i] = proc;

            proc->addLogEntry("(" + getCurrentTimestamp() + ") Core:" + std::to_string(i) + " Process " + proc->getProcessName() +" (PID:" + proc->getPid() + ") dispatched.");
        }
    }
}
