#include "ConsoleManager.h"
#include "ProcessConsole.h"
#include <iostream>
#include <memory>
#include <ctime>      
#include <iomanip>    
#include <sstream>  

ConsoleManager* ConsoleManager::instance = nullptr;

ConsoleManager::ConsoleManager() : activeConsole(nullptr), exitApp(false) {
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
    if (activeConsole != nullptr) {
    }
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


void ConsoleManager::createProcessConsole(const std::string& name) {
    if (doesProcessExist(name)) {
        std::cout << "Screen '" << name << "' already exists. Use 'screen -r " << name << "' to resume." << std::endl;
        return;
    }

    Process newProcess;
    newProcess.processName = name;
    newProcess.currentInstructionLine = 1;
    newProcess.totalInstructionLines = 100;
    newProcess.creationTime = getTimestamp();
    processes[name] = newProcess;

    processConsoleScreens[name] = std::make_unique<ProcessConsole>(processes[name]);

    setActiveConsole(processConsoleScreens[name].get());
}

void ConsoleManager::switchToProcessConsole(const std::string& name) {
    if (!doesProcessExist(name)) {
        std::cout << "Screen '" << name << "' not found." << std::endl;
        return;
    }

    if (processConsoleScreens.count(name)) {
        setActiveConsole(processConsoleScreens[name].get());
    } else {
        std::cout << "Process data for '" << name << "' found, but console screen not managed. Creating new screen." << std::endl;
        processConsoleScreens[name] = std::make_unique<ProcessConsole>(processes[name]);
        setActiveConsole(processConsoleScreens[name].get());
    }
}

AConsole* ConsoleManager::getMainConsole() const {
    return mainConsole.get();
}