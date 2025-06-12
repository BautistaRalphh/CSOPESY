#include "ConsoleManager.h"
#include <iostream>
#include <string>

int main() {
    int coreCount = 4;

    ConsoleManager* consoleManager = ConsoleManager::getInstance(coreCount);

    std::string command;

    while (!consoleManager->applicationExit()) {
        consoleManager->drawConsole();

        std::getline(std::cin, command);

        consoleManager->handleCommand(command);

        command.clear();
    }

    return 0;
}
