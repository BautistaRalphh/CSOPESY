#include "ConsoleManager.h"
#include <iostream>
#include <string>

int main() {
    ConsoleManager* consoleManager = ConsoleManager::getInstance();

    std::string command;

    while (!consoleManager->applicationExit()) {
        consoleManager->drawConsole();

        std::getline(std::cin, command);

        consoleManager->process(command);

        command.clear();
    }

    return 0;
}