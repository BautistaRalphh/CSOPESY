#include "MainConsole.h"
#include "ConsoleManager.h"
#include <regex>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <vector> 

MainConsole::MainConsole() : AConsole("MainConsole"), headerDisplayed(false) {}

void MainConsole::onEnabled() {
    system("cls");
}

void MainConsole::display() {
    if (!headerDisplayed) {
        displayHeader();
        headerDisplayed = true;
    }
    std::cout << std::endl;
    std::cout << "\033[0m" << "Enter command: ";
}

void MainConsole::handleCommand(const std::string& command) {
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
    std::regex screen_ls_regex(R"(^screen\s+-ls$)");
    std::smatch match;

    if (command == "initialize") {
        std::cout << "Scheduler initialization (not active yet)." << std::endl;
    } else if (std::regex_match(command, match, screen_cmd_regex)) {
        std::string option = match[1].str();     
        std::string processName = match[2].str(); 

        if (option == "-r") {
            ConsoleManager::getInstance()->switchToProcessConsole(processName);
        } else if (option == "-s") {
            std::filesystem::path logDirPath = "process_logs";
            if (!std::filesystem::exists(logDirPath)) {
                try {
                    std::filesystem::create_directory(logDirPath);
                } catch (const std::filesystem::filesystem_error& e) {
                    std::cerr << "Error creating directory 'process_logs': " << e.what() << std::endl;
                }
            }
            ConsoleManager::getInstance()->createProcessConsole(processName);

            ConsoleManager::getInstance()->switchToProcessConsole(processName);
        }
    } else if (std::regex_match(command, match, screen_ls_regex)) {
        auto allProcesses = ConsoleManager::getInstance()->getAllProcesses();

        std::vector<Process> activeProcesses;
        std::vector<Process> finishedProcesses;

        for (const auto& pair : allProcesses) {
            const Process& p = pair.second;
            if (static_cast<int>(p.getStatus()) == static_cast<int>(ProcessStatus::FINISHED)) {
                finishedProcesses.push_back(p);
            } else {
                activeProcesses.push_back(p);
            }
        }

        // --- Display Active Processes ---
        std::cout << "\n--- Active Processes ---" << std::endl;
        if (activeProcesses.empty()) {
            std::cout << "  No active processes found." << std::endl;
        } else {
            for (const auto& p : activeProcesses) {
                std::string statusStr;
                switch (p.getStatus()) {
                    case ProcessStatus::IDLE: statusStr = "IDLE"; break;
                    case ProcessStatus::RUNNING: statusStr = "RUNNING"; break;
                    case ProcessStatus::PAUSED: statusStr = "PAUSED"; break;
                    case ProcessStatus::FINISHED: statusStr = "FINISHED"; break; 
                }
                std::cout << "  " << p.getProcessName() 
                          << " (" << p.getCreationTime() << ") "
                          << "Status: " << statusStr
                          << " Core: " << (p.getCpuCoreExecuting() == -1 ? "N/A" : std::to_string(p.getCpuCoreExecuting()))
                          << " " << p.getCurrentCommandIndex() << "/" << p.getTotalInstructionLines() 
                          << std::endl;
            }
        }

        std::cout << "\n--- Finished Processes ---" << std::endl;
        if (finishedProcesses.empty()) {
            std::cout << "  No finished processes found." << std::endl;
        } else {
            for (const auto& p : finishedProcesses) {
                 std::cout << "  " << p.getProcessName() 
                          << " (" << p.getCreationTime() << ") "
                          << "Status: " << "FINISHED"
                          << " Core: " << (p.getCpuCoreExecuting() == -1 ? "N/A" : std::to_string(p.getCpuCoreExecuting()))
                          << " " << p.getCurrentCommandIndex() << "/" << p.getTotalInstructionLines() 
                          << std::endl;
            }
        }
        std::cout << std::flush; 
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