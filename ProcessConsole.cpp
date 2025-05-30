#include "ProcessConsole.h"
#include <iostream>
#include "ConsoleManager.h"

ProcessConsole::ProcessConsole(const Process& processData)
    : AConsole(processData.processName), currentProcessData(processData) {
}

void ProcessConsole::onEnabled() {
    std::cout << "\033[2J\033[1;1H"; 
    displayProcessInfo(); 
}


void ProcessConsole::display() {
    std::cout << std::endl;
    std::cout << "root:\\> "; 
}

void ProcessConsole::handleCommand(const std::string& command) {
    if (command == "process -smi") {
        std::cout << "\033[2J\033[1;1H";
        displayProcessInfo();
    } else if (command == "exit") {
        std::cout << "Exiting process screen for " << currentProcessData.processName << std::endl;
        ConsoleManager::getInstance()->setActiveConsole(ConsoleManager::getInstance()->getMainConsole());
    } else {
        std::cout << "Unknown command for process console: " << command << std::endl;
    }
}

void ProcessConsole::displayProcessInfo() {
    std::cout << "Name: " << currentProcessData.processName << std::endl;
    std::cout << "Current Instruction: " << currentProcessData.currentInstructionLine << std::endl;
    std::cout << "Total Instructions: " << currentProcessData.totalInstructionLines << std::endl;
    std::cout << "Creation Time: " << currentProcessData.creationTime << std::endl;
}