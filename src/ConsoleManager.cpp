#include "ConsoleManager.h"

#include "console/ProcessConsole.h"
#include "console/MainConsole.h"
#include "core/Process.h"
#include "core/Scheduler.h"
#include "memory/IMemoryAllocator.h"
#include "memory/FlatMemoryAllocator.h"
#include "memory/DemandPagingAllocator.h"
#include "memory/BackingStore.h"

#include <iostream>
#include <memory>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>

#include <map>

#include <filesystem>
#include <map>
#include <fstream>
#include <mutex>
#include <random>

ConsoleManager* ConsoleManager::instance = nullptr;
static std::atomic<int> autoProcessCounter(0); 
static std::atomic<long> pidCounter(0);

static std::string generatePid() {
    return "PID" + std::to_string(pidCounter.fetch_add(1));
}

static std::string generateAutoProcessName() {
    return "process_" + std::to_string(autoProcessCounter.fetch_add(1));
}

ConsoleManager::ConsoleManager()
    : activeConsole(nullptr),
      exitApp(false),
      processes(),
      finishedProcesses(),
      processConsoleScreens(),
      scheduler(nullptr), 
      schedulerStarted(false),
      batchProcessFrequency(0),
      batchGenThread(nullptr), 
      batchGenRunning(false),
      nextBatchTickTarget(0),
      minInstructionsPerProcess(0),
      maxInstructionsPerProcess(0),
      processDelayPerExecution(0),
      quantumCycles(0),
      mainConsole(std::make_unique<MainConsole>()),
      pendingProcesses()
{
    setActiveConsole(mainConsole.get());
}

ConsoleManager* ConsoleManager::getInstance() {
    if (instance == nullptr) {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        if (instance == nullptr) {
            instance = new ConsoleManager();
        }
    }
    return instance;
}

void ConsoleManager::cleanupInstance() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

void ConsoleManager::setActiveConsole(AConsole* console) {
    if (console == nullptr) {
        std::cerr << "[ERROR] Attempted to set null console as active" << std::endl;
        return;
    }
    
    system("cls");
    activeConsole = console;
    
    if (activeConsole != nullptr) {
        try {
            activeConsole->onEnabled();
        } catch (...) {
            std::cerr << "[ERROR] Exception occurred while enabling console" << std::endl;
            activeConsole = nullptr;
        }
    }
}

void ConsoleManager::handleCommand(const std::string& command) {
    if (activeConsole) {
        activeConsole->handleCommand(command);
    }
}

void ConsoleManager::drawConsole() {
    if (activeConsole) {
        activeConsole->display();
    }
}

void ConsoleManager::setExitApp(bool val) {
    exitApp = val;
}

bool ConsoleManager::getExitApp() const {
    return exitApp;
}

std::string ConsoleManager::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = *std::localtime(&now_c);

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S%p", &localTime);
    return std::string(buffer);
}

bool ConsoleManager::doesProcessExist(const std::string& name) const {
    return processes.count(name) > 0;
}

std::shared_ptr<const Process> ConsoleManager::getProcess(const std::string& name) const {
    auto it = processes.find(name);
    return (it != processes.end()) ? it->second : nullptr;
}

std::shared_ptr<Process> ConsoleManager::getProcessMutable(const std::string& name) {
    auto it = processes.find(name);
    return (it != processes.end()) ? it->second : nullptr;
}

const std::map<std::string, std::shared_ptr<Process>>& ConsoleManager::getAllProcesses() const {
    static std::map<std::string, std::shared_ptr<Process>> all;
    all.clear();
    all.insert(processes.begin(), processes.end());
    all.insert(finishedProcesses.begin(), finishedProcesses.end());
    return all;
}

bool ConsoleManager::createProcessConsole(const std::string& name) {
    if (doesProcessExist(name)) {
        std::cout << "Screen '" << name << "' already exists. Use 'screen -r " << name << "' to resume." << std::endl;
        return false;
    }

    auto newProcess = std::make_shared<Process>(name, generatePid(), getTimestamp());
    newProcess->setStatus(ProcessStatus::NEW);
    newProcess->setCpuCoreExecuting(-1);
    newProcess->setFinishTime("N/A");

    if (minInstructionsPerProcess == 0 || maxInstructionsPerProcess == 0 || minInstructionsPerProcess > maxInstructionsPerProcess) {
        std::cerr << "[ERROR] Process instruction range (min-ins, max-ins) is not properly initialized or invalid. Defaulting to 100 instructions." << std::endl;
        newProcess->generateRandomCommands(100);
    } else {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> distrib(minInstructionsPerProcess, maxInstructionsPerProcess);
        uint32_t numInstructions = distrib(gen);
        newProcess->generateRandomCommands(numInstructions);
    }

    static std::random_device rd_mem; 
    static std::mt19937 gen_mem(rd_mem());
    std::uniform_int_distribution<uint32_t> memDistrib(minMemoryPerProcess, maxMemoryPerProcess);
    uint32_t memoryRequired = memDistrib(gen_mem);

    if (memoryAllocator) {
        newProcess->setMemory(memoryRequired, 0);
        
        if (scheduler && scheduler->getAlgorithmType() == SchedulerAlgorithmType::rr) {
            processes[name] = newProcess; 
            scheduler->addProcessToRRPendingQueue(newProcess);
            newProcess->addLogEntry("(" + getTimestamp() + ") Process " + newProcess->getProcessName() +
                                     " (PID:" + newProcess->getPid() + ") created and added to RR pending queue (awaiting memory allocation).");
            processConsoleScreens[name] = std::make_unique<ProcessConsole>(newProcess); 
            return true;
        } else { 
            void* allocResult = memoryAllocator->allocate(newProcess);
            if (!allocResult) {
                //std::cerr << "[ERROR] Memory allocation failed for process '" << name << "' (FCFS/immediate allocation required). Process not created/queued." << std::endl;
                return false;
            } else {
                processes[name] = newProcess;
                if (scheduler) {
                    scheduler->addProcess(newProcess); 
                } else {
                    std::cerr << "[WARNING] Scheduler not initialized. Process '" << name << "' created but not queued for execution (FCFS scenario)." << std::endl;
                }
                processConsoleScreens[name] = std::make_unique<ProcessConsole>(newProcess);
                return true;
            }
        }
    }
    return false;
}

bool ConsoleManager::createCustomProcessConsole(const std::string& name, const std::vector<std::string>& instructions) {
    if (doesProcessExist(name)) {
        std::cout << "Screen '" << name << "' already exists. Use 'screen -r " << name << "' to resume." << std::endl;
        return false;
    }

    auto newProcess = std::make_shared<Process>(name, generatePid(), getTimestamp());
    newProcess->setStatus(ProcessStatus::NEW);
    newProcess->setCpuCoreExecuting(-1);
    newProcess->setFinishTime("N/A");

    for (const auto& instruction : instructions) {
        newProcess->addCommand(instruction);
    }

    static std::random_device rd_mem; 
    static std::mt19937 gen_mem(rd_mem());
    std::uniform_int_distribution<uint32_t> memDistrib(minMemoryPerProcess, maxMemoryPerProcess);
    uint32_t memoryRequired = memDistrib(gen_mem);

    if (memoryAllocator) {
        newProcess->setMemory(memoryRequired, 0);
        
        if (scheduler && scheduler->getAlgorithmType() == SchedulerAlgorithmType::rr) {
            processes[name] = newProcess; 
            scheduler->addProcessToRRPendingQueue(newProcess);
            newProcess->addLogEntry("(" + getTimestamp() + ") Process " + newProcess->getProcessName() +
                                     " (PID:" + newProcess->getPid() + ") created with custom instructions and added to RR pending queue (awaiting memory allocation).");
            processConsoleScreens[name] = std::make_unique<ProcessConsole>(newProcess); 
            return true;
        } else { 
            void* allocResult = memoryAllocator->allocate(newProcess);
            if (!allocResult) {
                //std::cerr << "[ERROR] Memory allocation failed for process '" << name << "' (FCFS/immediate allocation required). Process not created/queued." << std::endl;
                return false;
            } else {
                processes[name] = newProcess;
                if (scheduler) {
                    scheduler->addProcess(newProcess); 
                } else {
                    std::cerr << "[WARNING] Scheduler not initialized. Process '" << name << "' created but not queued for execution (FCFS scenario)." << std::endl;
                }
                processConsoleScreens[name] = std::make_unique<ProcessConsole>(newProcess);
                return true;
            }
        }
    }
    return false;
}

void ConsoleManager::switchToProcessConsole(const std::string& name) {
    std::shared_ptr<Process> processData;
    bool isFinished = false;
    
    auto itProcess = processes.find(name);
    if (itProcess != processes.end()) {
        processData = itProcess->second;
    } else {
        auto itFinished = finishedProcesses.find(name);
        if (itFinished != finishedProcesses.end()) {
            processData = itFinished->second;
            isFinished = true;
        }
    }

    if (!processData) {
        std::cout << "Screen '" << name << "' not found." << std::endl;
        return;
    }

    if (isFinished) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm localTime = *std::localtime(&now_c);
        
        char timeBuffer[32];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &localTime);
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> hexDistrib(0x1000, 0xFFFF);
        uint32_t memAddr = hexDistrib(gen);
        
        std::cout << "Process " << name << " shut down due to memory access violation error that occurred at " 
                  << timeBuffer << ". 0x" << std::hex << std::uppercase << memAddr << std::dec 
                  << " invalid." << std::endl;
        return;
    }

    auto itScreen = processConsoleScreens.find(name);
    if (itScreen != processConsoleScreens.end()) {
        ProcessConsole* pc = itScreen->second.get();
        pc->updateProcessData(processData);
        setActiveConsole(pc);
    } else {
        std::cout << "Creating console for process '" << name << "'." << std::endl;
        processConsoleScreens[name] = std::make_unique<ProcessConsole>(processData);
        setActiveConsole(processConsoleScreens[name].get());
    }
}

void ConsoleManager::cleanupTerminatedProcessConsole(const std::string& name) {
    auto finishedIt = finishedProcesses.find(name);
    if (finishedIt != finishedProcesses.end()) {
        auto consoleIt = processConsoleScreens.find(name);
        if (consoleIt != processConsoleScreens.end()) {
            processConsoleScreens.erase(consoleIt);
            std::cout << "Console for terminated process '" << name << "' has been cleaned up." << std::endl;
        }
        
    }
}

AConsole* ConsoleManager::getMainConsole() const {
    return mainConsole.get();
}

bool ConsoleManager::readConfigFile(const std::string& filename, std::map<std::string, std::string>& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);

            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            config[key] = value;
        }
    }
    file.close();
    return true;
}

void ConsoleManager::createBatchProcess() {
    static std::mutex createBatchProcessMtx;
    std::lock_guard<std::mutex> lock(createBatchProcessMtx);
    std::string processName = generateAutoProcessName();
    createProcessConsole(processName);
}

void ConsoleManager::batchGenLoop() {
    while (batchGenRunning.load()) {
        if (scheduler) {
            long long currentSimulatedTime = scheduler->getSimulatedTime();

            if (currentSimulatedTime >= nextBatchTickTarget) {
                createBatchProcess();
                nextBatchTickTarget += batchProcessFrequency;

                while (batchGenRunning.load() &&
                       (currentSimulatedTime = scheduler->getSimulatedTime()) >= nextBatchTickTarget) {
                    createBatchProcess();
                    nextBatchTickTarget += batchProcessFrequency;

                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); //Lower for faster batch gen
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void ConsoleManager::startBatchGen() {
    if (batchGenRunning.load()) {
        std::cout << "[INFO] Batch process generation thread already running." << std::endl;
        return;
    }
    if (!scheduler || scheduler->getAlgorithmType() == SchedulerAlgorithmType::NONE || batchProcessFrequency <= 0) {
        std::cerr << "[ERROR] Cannot start batch process generation: Scheduler not initialized or frequency not valid." << std::endl;
        return;
    }

    batchGenRunning.store(true);
    nextBatchTickTarget = scheduler->getSimulatedTime() + batchProcessFrequency;

    batchGenThread = std::make_unique<std::thread>(&ConsoleManager::batchGenLoop, this);
    std::cout << "[INFO] Batch process generation thread started." << std::endl;
}

void ConsoleManager::stopBatchGen() {
    if (batchGenRunning.load()) {
        batchGenRunning.store(false);

        if (batchGenThread && batchGenThread->joinable()) {
            batchGenThread->join(); 
            batchGenThread.reset();
            std::cout << "[INFO] Batch process generation thread stopped." << std::endl;
        } 
    }
}

void ConsoleManager::initializeSystem(
    int numCpus,
    SchedulerAlgorithmType type,
    int batchFreq,
    uint32_t minIns,
    uint32_t maxIns,
    uint32_t delaysPerExec,
    uint32_t quantumCyc,
    uint32_t maxOverallMem,
    uint32_t memPerFrame,
    uint32_t minMemPerProc,
    uint32_t maxMemPerProc
) {
    if (schedulerStarted.load()) {
        return;
    }

    if (batchGenRunning.load()) {
        stopBatchGen();
    }

    if (scheduler) {
        scheduler.reset();
    }

    scheduler = std::make_unique<Scheduler>(numCpus);
    scheduler->setAlgorithmType(type);
    scheduler->setDelaysPerExecution(delaysPerExec);
    scheduler->setQuantumCycles(quantumCyc);

    this->minInstructionsPerProcess = minIns;
    this->maxInstructionsPerProcess = maxIns;
    this->processDelayPerExecution = delaysPerExec;
    this->quantumCycles = quantumCyc;
    this->maxOverallMemory = maxOverallMem;
    this->memoryPerFrame = memPerFrame;
    this->minMemoryPerProcess = minMemPerProc;
    this->maxMemoryPerProcess = maxMemPerProc;

    memoryAllocator = std::make_unique<DemandPagingAllocator>(maxOverallMemory, memoryPerFrame, DemandPagingAllocator::PageReplacementPolicy::FIFO);
    batchProcessFrequency = batchFreq;

    if (scheduler) {
        scheduler->setProcessTerminationCallback([this](std::shared_ptr<Process> proc) {
            std::string name = proc->getProcessName();

            auto it = processes.find(name);
            if (it != processes.end()) {
                if (memoryAllocator) {
                    memoryAllocator->deallocate(it->second);
                }
                finishedProcesses[name] = it->second;
                processes.erase(it);
            }

            auto consoleIt = processConsoleScreens.find(name);
            if (consoleIt != processConsoleScreens.end()) {
                auto finishedProcessIt = finishedProcesses.find(name);
                if (finishedProcessIt != finishedProcesses.end()) {
                    consoleIt->second->updateProcessData(finishedProcessIt->second);
                }
                
                if (activeConsole != consoleIt->second.get()) {
                    processConsoleScreens.erase(consoleIt);
                }
            }
        });

        nextBatchTickTarget = scheduler->getSimulatedTime() + batchProcessFrequency;
    } else {
        nextBatchTickTarget = batchProcessFrequency;
    }

}

void ConsoleManager::startScheduler() {
    if (!scheduler || scheduler->getAlgorithmType() == SchedulerAlgorithmType::NONE) {
        std::cerr << "Error: Scheduler is not initialized or algorithm is not set. Use 'initialize' command first." << std::endl;
        return;
    }

    if (!schedulerStarted.load()) {
        // Start the scheduler if it's not running
        scheduler->start();
        schedulerStarted.store(true);
    }

    // Start batch generation if frequency is set and not already running
    if (batchProcessFrequency > 0) {
        startBatchGen();
    }
}

void ConsoleManager::stopScheduler() {
    if (schedulerStarted.load()) {
        stopBatchGen();

        if (scheduler) {
            scheduler->stop();
            schedulerStarted.store(false);
        }

    } else if (!scheduler) {
        std::cerr << "[ERROR] Scheduler is not initialized." << std::endl;
    }
}

Scheduler* ConsoleManager::getScheduler() const {
    return scheduler.get();
}

std::vector<std::shared_ptr<Process>> ConsoleManager::getProcesses() const {
    std::vector<std::shared_ptr<Process>> list;
    for (const auto& [_, proc] : processes) {
        list.push_back(proc);
    }
    return list;
}
