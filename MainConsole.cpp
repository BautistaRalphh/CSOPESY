#include "MainConsole.h"
#include "ConsoleManager.h"
#include <regex>

MainConsole::MainConsole() : AConsole("MainConsole"), headerDisplayed(false) {}

void MainConsole::onEnabled() {
    std::cout << "\033[2J\033[1;1H";

    if (!headerDisplayed) {
        displayHeader();
        headerDisplayed = true;
    }
}

void MainConsole::display() {
    std::cout << std::endl;
    std::cout << "\033[0m" << "Enter command: ";
}

void MainConsole::process(const std::string& command) {
    handleMainCommands(command);
}

void MainConsole::displayHeader() {
    std::cout << "  _____  _____  ____  _____  ______  _______    __" << std::endl;
    std::cout << " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____\\ \\  / /" << std::endl;
    std::cout << "| |    | (___ | |  | | |__) | |__  | (___  \\ \\/ / " << std::endl;
    std::cout << "| |     \\___ \\| |  | | ___/ |  __|  \\___ \\  \\  /  " << std::endl;
    std::cout << "| |____ ____) | |__| | |    | |____ ____) |  | |   " << std::endl;
    std::cout << " \\_____|_____/ \\____/|_|    |______|_____/   |_|   " << std::endl;
    std::cout << "\033[32mHello, welcome to CSOPESY command line\033[0m" << std::endl;
    std::cout << "\033[33mType 'exit' to quit, 'clear' to clear the screen\033[0m" << std::endl;
}

void MainConsole::handleMainCommands(const std::string& command) {
    std::regex screen_cmd_regex(R"(^screen\s+(-r|-s)\s+(\w+)$)");
    std::smatch match;

    if (command == "initialize") {
        std::cout << "'initialize' command recognized. Doing something." << std::endl;
    } else if (std::regex_match(command, match, screen_cmd_regex)) {
        std::string option = match[1].str();
        std::string processName = match[2].str();

        if (option == "-r") {
            ConsoleManager::getInstance()->switchToProcessConsole(processName);
        } else if (option == "-s") {
            ConsoleManager::getInstance()->createProcessConsole(processName);
        }
    } else if (command == "scheduler-test") {
        std::cout << "'scheduler-test' command recognized. Doing something." << std::endl;
    } else if (command == "scheduler-stop") {
        std::cout << "'scheduler-stop' command recognized. Doing something." << std::endl;
    } else if (command == "report-util") {
        std::cout << "'report-util' command recognized. Doing something." << std::endl;
    } else if (command == "clear") {
        onEnabled(); 
    } else if (command == "exit") {
        std::cout << "Exiting CSOPESY." << std::endl;
        ConsoleManager::getInstance()->setExitApp(true);
    } else {
        std::cout << "Unknown command: " << command << std::endl;
    }
}