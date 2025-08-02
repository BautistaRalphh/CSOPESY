#include "ProcessConsole.h"
#include "ConsoleManager.h"
#include "core/Process.h"  
#include "memory/DemandPagingAllocator.h"

#include <iostream>
#include <iomanip>          
#include <cstdlib>

ProcessConsole::ProcessConsole(std::shared_ptr<Process> processData) 
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
        std::shared_ptr<Process> latestData = ConsoleManager::getInstance()->getProcessMutable(currentProcessData->getProcessName());
        if (latestData) {
            updateProcessData(latestData); 
        }
        system("cls");
        displayProcessInfo(); 
    } else if (command == "exit") {
        std::cout << "Exiting process screen for " << currentProcessData->getProcessName() << std::endl;
        std::cout << std::flush;

        if (currentProcessData->getStatus() == ProcessStatus::TERMINATED) {
            std::cout << "This process has terminated. Cleaning up console..." << std::endl;
            ConsoleManager::getInstance()->cleanupTerminatedProcessConsole(currentProcessData->getProcessName());
        }

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
    std::cout << "Memory Required: " << currentProcessData->getMemoryRequired() << " bytes" << std::endl;
    std::cout << "Pages Allocated: " << currentProcessData->getPagesAllocated() << std::endl;
    
    // Get additional memory information from the memory allocator
    auto* memoryAllocator = ConsoleManager::getInstance()->getMemoryAllocator();
    if (memoryAllocator) {
        // Try to cast to DemandPagingAllocator to access specific methods
        auto* demandPagingAllocator = dynamic_cast<DemandPagingAllocator*>(memoryAllocator);
        if (demandPagingAllocator) {
            std::string pid = currentProcessData->getPid();
            int pagesInPhysical = demandPagingAllocator->getPagesInPhysicalMemory(pid);
            int pagesInBacking = demandPagingAllocator->getPagesInBackingStore(pid);
            
            std::cout << "Pages in Physical Memory: " << pagesInPhysical << std::endl;
            std::cout << "Pages in Backing Store: " << pagesInBacking << std::endl;
        }
    }
    
    std::cout << "\033[0m";
    std::cout << std::endl;

    const auto& logs = currentProcessData->getLogEntries();
    const size_t maxLogsToDisplay = 100;

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

void ProcessConsole::updateProcessData(std::shared_ptr<Process> newData) {
    currentProcessData = newData; 
}
