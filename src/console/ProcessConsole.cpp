#include "ProcessConsole.h"
#include "ConsoleManager.h"
#include "core/Process.h"  

#include <iostream>
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
    if (command == "process-smi") {
        Process* latestData = ConsoleManager::getInstance()->getProcessMutable(currentProcessData->getProcessName());
        if (latestData) {
            updateProcessData(latestData); 
        }
        system("cls");
        displayProcessInfo(); 
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
        case ProcessStatus::READY: std::cout << "READY"; break;
        case ProcessStatus::RUNNING: std::cout << "\033[35mRUNNING\033[36m"; break;
        case ProcessStatus::TERMINATED: std::cout << "\033[31mTERMINATED\033[36m"; break;
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

    const auto& logs = currentProcessData->getLogEntries();
    const size_t maxLogsToDisplay = 15;

    if (!logs.empty()) {
        std::cout << "Logs:\n";

        size_t startIndex = logs.size() > maxLogsToDisplay ? logs.size() - maxLogsToDisplay : 0;
        for (size_t i = startIndex; i < logs.size(); ++i) {
            std::cout << "  " << logs[i] << std::endl;
        }

        if (logs.size() > maxLogsToDisplay) {
            std::cout << "  ... (showing last " << maxLogsToDisplay << " of " << logs.size() << " logs)\n";
        }
    } else {
        std::cout << "Logs: No logs recorded.\n";
    }
}

void ProcessConsole::updateProcessData(Process* newData) {
    currentProcessData = newData; 
}
