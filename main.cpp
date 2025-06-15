#include "ConsoleManager.h"
#include <iostream>
#include <string>

std::atomic<long long> cpuCycles(0);

int main() {
    ConsoleManager* consoleManager = ConsoleManager::getInstance();
    consoleManager->setCpuCyclesCounter(&cpuCycles);

    std::string command;

    while (!consoleManager->getExitApp()) {

        cpuCycles.fetch_add(1);

        consoleManager->drawConsole();

        std::getline(std::cin, command);

        consoleManager->handleCommand(command);

        command.clear();
    }
    
    return 0;
}
