#include "ConsoleManager.h"
#include "ProcessConsole.h"
#include "MainConsole.h" 
#include "Process.h"   
#include "Scheduler.h"
#include <iostream>
#include <memory>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <random>    
#include <filesystem>

ConsoleManager* ConsoleManager::instance = nullptr;

static std::atomic<long> pidCounter(0);

ConsoleManager::ConsoleManager() : activeConsole(nullptr), exitApp(false), schedulerStarted(false) {
    mainConsole = std::make_unique<MainConsole>();
    scheduler = std::make_unique<Scheduler>(4); 
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

bool ConsoleManager::applicationExit() const {
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

void ConsoleManager::createProcessConsole(const std::string& name) {
    if (doesProcessExist(name)) {
        std::cout << "Screen '" << name << "' already exists. Use 'screen -r " << name << "' to resume." << std::endl;
        return;
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

void ConsoleManager::startScheduler() {
    if (scheduler && !schedulerStarted) {
        scheduler->start();
        schedulerStarted = true;
    } else if (!scheduler) {
        std::cerr << "[ERROR] Scheduler is not initialized." << std::endl;
    } else {
        std::cout << "Scheduler is already running." << std::endl;
    }
}

Scheduler* ConsoleManager::getScheduler() {
    return scheduler.get();
}