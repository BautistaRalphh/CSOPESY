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

ConsoleManager* ConsoleManager::instance = nullptr;

static std::atomic<long> pidCounter(0);

ConsoleManager::ConsoleManager() : activeConsole(nullptr), exitApp(false), schedulerStarted(false) {
    mainConsole = std::make_unique<MainConsole>();
    setActiveConsole(mainConsole.get());
}

ConsoleManager* ConsoleManager::getInstance() {
    if (instance == nullptr) {
        instance = new ConsoleManager(); 
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
    std::time_t now = std::time(nullptr);
    std::tm* ltm = std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(ltm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
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

static std::string generatePid() {
    return "PID" + std::to_string(pidCounter.fetch_add(1));
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

    newProcess.generateDummyPrintCommands(100, "Hello world from ");

    processes[name] = newProcess; 

    Process* processInMap = &(processes.at(name)); 

    scheduler->addProcess(processInMap);

    processConsoleScreens[name] = std::make_unique<ProcessConsole>(processInMap); 

    std::cout << "Process '" << name << "' (PID: " << processInMap->getPid() << ") created." << std::endl;
    // Note: We are NOT immediately switching to the process console here for `screen -s`.
    // The MainConsole's handleCommand will redraw its prompt unless switched.
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

void ConsoleManager::initializeSystem(int numCpus, SchedulerAlgorithmType type) {
    if (scheduler && scheduler->isRunning()) {
        std::cout << "Scheduler is already running. Please stop it before re-initializing." << std::endl;
        return;
    }

    if (scheduler) {
        scheduler.reset(); 
        schedulerStarted = false; 
    }
    
    scheduler = std::make_unique<Scheduler>(numCpus);
    
    scheduler->setAlgorithmType(type);

    std::cout << "System initialized with " << numCpus << " CPUs and scheduler type: ";
    switch(type) {
        case SchedulerAlgorithmType::FCFS:
            std::cout << "FCFS";
            break;
        case SchedulerAlgorithmType::NONE:
            std::cout << "NONE (algorithm not set)";
            break;
        // Add cases for other algorithm types (e.g., ROUND_ROBIN) here
        // case SchedulerAlgorithmType::ROUND_ROBIN:
        //     std::cout << "Round Robin";
        //     break;
    }
    std::cout << "." << std::endl;

}

void ConsoleManager::startScheduler() {
    if (schedulerStarted) {
        std::cout << "Scheduler is already running." << std::endl;
        return;
    }
    
    if (!scheduler || scheduler->getAlgorithmType() == SchedulerAlgorithmType::NONE) {
        std::cerr << "Error: Scheduler is not initialized or algorithm is not set. Use 'initialize' command first." << std::endl;
        return;
    }

    if (scheduler) {
        scheduler->start();
        schedulerStarted = true;
        std::cout << "Scheduler started successfully!" << std::endl;
    } else {
        std::cerr << "Error: Scheduler instance is not available." << std::endl;
    }
}

void ConsoleManager::stopScheduler() {
    if (scheduler && schedulerStarted) {
        scheduler->stop();
        schedulerStarted = false;

        for (auto& pair : processes) {
            Process& p = pair.second; 
            if (p.getStatus() == ProcessStatus::RUNNING) {
                p.setStatus(ProcessStatus::PAUSED);
                p.setCpuCoreExecuting(-1); 
            }
        }
        
    } else if (!scheduler) {
        std::cerr << "[ERROR] Scheduler is not initialized." << std::endl;
    } else {
        std::cout << "Scheduler is not currently running." << std::endl;
    }
}

Scheduler* ConsoleManager::getScheduler() const {
    return scheduler.get();
}
