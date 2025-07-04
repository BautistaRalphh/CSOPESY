#include "ConsoleManager.h"
#include <iostream>
#include <string>

int main() {
    ConsoleManager* consoleManager = ConsoleManager::getInstance();

    std::string command;

    while (!consoleManager->getExitApp()) {

        consoleManager->drawConsole();

        std::getline(std::cin, command);

        consoleManager->handleCommand(command);

        command.clear();
    }
    if (consoleManager->getScheduler() && consoleManager->getScheduler()->isRunning()) {
        consoleManager->stopScheduler();
    }
    consoleManager->stopBatchGen();

    return 0;
}
