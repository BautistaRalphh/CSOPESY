#include "ProcessConsole.h"
#include <iostream>
#include "ConsoleManager.h"
#include "Process.h"        
#include <iomanip>          
#include <cstdlib>

ProcessConsole::ProcessConsole(Process* processData) 
    : AConsole(processData->getProcessName()), currentProcessData(processData) {}

void ProcessConsole::onEnabled() {
    system("cls"); 
    displayProcessInfo(); 
}

void ProcessConsole::display() {
    std::cout << std::endl;
    std::cout << "root:\\> "; 
    std::cout << std::flush;
}

void ProcessConsole::handleCommand(const std::string& command) {
    if (command == "process -smi") {
        Process* latestData = ConsoleManager::getInstance()->getProcessMutable(currentProcessData->getProcessName());
        if (latestData) {
            updateProcessData(latestData); 
        }
        system("cls");
        displayProcessInfo(); 
        display();
    } else if (command == "exit") {
        std::cout << "Exiting process screen for " << currentProcessData->getProcessName() << std::endl;
        std::cout << std::flush;

        ConsoleManager::getInstance()->setActiveConsole(ConsoleManager::getInstance()->getMainConsole());
    } else {
        std::cout << "Unknown command for process console: " << command << std::endl;
    }
}

void ProcessConsole::displayProcessInfo() {
    std::cout << "Process Information" << std::endl;
    std::cout << "Name: " << currentProcessData->getProcessName() << std::endl;
    std::cout << "PID: " << currentProcessData->getPid() << std::endl;
    std::cout << "Status: ";
    switch (currentProcessData->getStatus()) { 
        case ProcessStatus::NEW: std::cout << "\033[36mNEW"; break;
        case ProcessStatus::IDLE: std::cout << "IDLE"; break;
        case ProcessStatus::RUNNING: std::cout << "\033[35mRUNNING\033[36m"; break; 
        case ProcessStatus::FINISHED: std::cout << "\033[31mFINISHED\033[36m"; break; 
        case ProcessStatus::PAUSED: std::cout << "\033[33mPAUSED\033[36m"; break; 
    }
    std::cout << "\033[0m" << std::endl; 
    std::cout << "CPU Core: " << currentProcessData->getCpuCoreExecuting() << std::endl;
    std::cout << "Commands Executed: " << currentProcessData->getCurrentCommandIndex() << std::endl;
    std::cout << "Total Commands: " << currentProcessData->getTotalInstructionLines() << std::endl;
    std::cout << "Creation Time: " << currentProcessData->getCreationTime() << std::endl;
    std::cout << "Finish Time: " << currentProcessData->getFinishTime() << std::endl; 
    std::cout << "\033[0m";
    std::cout << std::endl;
}

void ProcessConsole::updateProcessData(Process* newData) {
    currentProcessData = newData; 
}
