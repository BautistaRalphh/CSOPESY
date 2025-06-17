#include "ConsoleManager.h"

#include "console/ProcessConsole.h"
#include "console/MainConsole.h"
#include "core/Process.h"
#include "core/Scheduler.h"

#include <iostream>
#include <memory>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <random>
#include <filesystem>
#include <map>
#include <fstream>
#include <mutex>

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
      processConsoleScreens(),
      scheduler(nullptr),
      schedulerStarted(false),
      batchProcessFrequency(0),
      batchGenThread(nullptr),
      batchGenRunning(false),
      nextBatchTickTarget(0),
      cpuCyclesPtr(nullptr),
      mainConsole(std::make_unique<MainConsole>())
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

void ConsoleManager::setActiveConsole(AConsole* console) {
    system("cls");
    activeConsole = console;
    if (activeConsole != nullptr) {
        activeConsole->onEnabled();
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

const Process* ConsoleManager::getProcess(const std::string& name) const {
    auto it = processes.find(name);
    if (it != processes.end()) {
        return &(it->second);
    }
    return nullptr;
}

Process* ConsoleManager::getProcessMutable(const std::string& name) {
    auto it = processes.find(name);
    if (it != processes.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::map<std::string, Process> ConsoleManager::getAllProcesses() const {
    return processes;
}

bool ConsoleManager::createProcessConsole(const std::string& name) {
    if (doesProcessExist(name)) {
        std::cout << "Screen '" << name << "' already exists. Use 'screen -r " << name << "' to resume." << std::endl;
        return false;
    }

    Process newProcess(name, generatePid(), getTimestamp());
    newProcess.setStatus(ProcessStatus::NEW);
    newProcess.setCpuCoreExecuting(-1);
    newProcess.setFinishTime("N/A");

    newProcess.generateDummyCommands(1);

    processes[name] = newProcess;

    Process* processInMap = &(processes.at(name));

    if (scheduler) {
        scheduler->addProcess(processInMap);
    } else {
        std::cerr << "[WARNING] Scheduler not initialized. Process '" << name << "' created but not queued for execution." << std::endl;
    }

    processConsoleScreens[name] = std::make_unique<ProcessConsole>(processInMap);
    return true;
}

void ConsoleManager::switchToProcessConsole(const std::string& name) {
    if (!doesProcessExist(name)) {
        std::cout << "Screen '" << name << "' not found." << std::endl;
        return;
    }

    Process* processData = getProcessMutable(name);
    if (!processData) {
        std::cout << "Error: Process data for '" << name << "' not found internally (mutable access failed)." << std::endl;
        return;
    }

    auto it = processConsoleScreens.find(name);
    if (it != processConsoleScreens.end()) {
        ProcessConsole* pc = it->second.get();
        pc->updateProcessData(processData);
        setActiveConsole(pc);
    } else {
        std::cout << "Process data for '" << name << "' found, but console screen not managed. Creating new screen." << std::endl;
        processConsoleScreens[name] = std::make_unique<ProcessConsole>(processData);
        setActiveConsole(processConsoleScreens[name].get());
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

void ConsoleManager::setCpuCyclesCounter(std::atomic<long long>* counterPtr) {
    cpuCyclesPtr = counterPtr;
}

void ConsoleManager::createBatchProcess() {
    static std::mutex createBatchProcessMtx;
    std::lock_guard<std::mutex> lock(createBatchProcessMtx);
    std::string processName = generateAutoProcessName();
    createProcessConsole(processName);
}

void ConsoleManager::batchGenLoop() {
    while (batchGenRunning.load()) {
        if (cpuCyclesPtr) {
            long long currentCpuCycles = cpuCyclesPtr->load();

            if (currentCpuCycles >= nextBatchTickTarget) {
                createBatchProcess();
                nextBatchTickTarget += batchProcessFrequency;

                while (currentCpuCycles >= nextBatchTickTarget) {
                    createBatchProcess();
                    nextBatchTickTarget += batchProcessFrequency;
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
    if (!cpuCyclesPtr || batchProcessFrequency <= 0) {
        std::cerr << "[ERROR] Cannot start batch process generation: CPU Cycles counter not set or frequency not valid." << std::endl;
        return;
    }

    batchGenRunning.store(true);
    nextBatchTickTarget = cpuCyclesPtr->load() + batchProcessFrequency;

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

void ConsoleManager::initializeSystem(int numCpus, SchedulerAlgorithmType type, int batchFreq) {
    if (schedulerStarted.load()) {
        std::cout << "Scheduler is already running. Please stop it before re-initializing." << std::endl;
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

    batchProcessFrequency = batchFreq;

    if (cpuCyclesPtr) { 
        nextBatchTickTarget = cpuCyclesPtr->load() + batchProcessFrequency;
    } else {
        nextBatchTickTarget = batchProcessFrequency;
    }

    std::cout << "System initialized with " << numCpus << " CPUs and scheduler type: ";
    switch(type) {
        case SchedulerAlgorithmType::FCFS:
            std::cout << "FCFS";
            break;
        case SchedulerAlgorithmType::NONE:
            std::cout << "NONE (algorithm not set)";
            break;
    }
    std::cout << "." << std::endl;

    if (batchProcessFrequency > 0) {
        std::cout << "Batch process generation configured to run every " << batchProcessFrequency << " CPU Cycles." << std::endl;
    } else {
        std::cout << "Batch process generation is disabled." << std::endl;
    }
}

void ConsoleManager::startScheduler() {
    if (schedulerStarted.load()) {
        std::cout << "Scheduler is already running." << std::endl;
        return;
    }

    if (!scheduler || scheduler->getAlgorithmType() == SchedulerAlgorithmType::NONE) {
        std::cerr << "Error: Scheduler is not initialized or algorithm is not set. Use 'initialize' command first." << std::endl;
        return;
    }

    scheduler->start();
    schedulerStarted.store(true);

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

        for (auto& pair : processes) {
            Process& p = pair.second;
            if (p.getStatus() == ProcessStatus::RUNNING) {
                p.setStatus(ProcessStatus::PAUSED);
                p.setCpuCoreExecuting(-1);
            }
        }
        std::cout << "Scheduler stopped." << std::endl;
    } else if (!scheduler) {
        std::cerr << "[ERROR] Scheduler is not initialized." << std::endl;
    } else {
        std::cout << "Scheduler is not currently running." << std::endl;
    }
}

Scheduler* ConsoleManager::getScheduler() const {
    return scheduler.get();
}