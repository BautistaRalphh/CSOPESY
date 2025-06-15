#include "MainConsole.h"
#include "ConsoleManager.h"
#include "Process.h"      
#include "core/Scheduler.h"

#include <regex>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <vector> 

MainConsole::MainConsole() : AConsole("MainConsole"), headerDisplayed(false), initialized(false) {}

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
    if (command == "exit") {
        std::cout << "Exiting CSOPESY." << std::endl;
        ConsoleManager::getInstance()->setExitApp(true);
        ConsoleManager::getInstance()->stopBatchGen();
        return;
    }

    if (!initialized) {
        if (command == "initialize") {
            std::map<std::string, std::string> config;
            if (!ConsoleManager::getInstance()->readConfigFile("config.txt", config)) {
                std::cerr << "Failed to load system configuration from config.txt." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            int numCpus = 0;
            try {
                numCpus = std::stoi(config["num-cpu"]);
                if (numCpus <= 0) {
                    std::cerr << "Error: 'num-cpu' must be a positive integer in config.txt. Found: " << config["num-cpu"] << std::endl;
                    std::cout << "Initialization failed." << std::endl;
                    return;
                }
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error: Invalid 'num-cpu' value in config.txt. Must be an integer." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            } catch (const std::out_of_range& e) {
                std::cerr << "Error: 'num-cpu' value out of range in config.txt." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            std::string schedulerTypeStr = config["scheduler"];
            SchedulerAlgorithmType algoType = SchedulerAlgorithmType::NONE;

            if (schedulerTypeStr == "FCFS") {
                algoType = SchedulerAlgorithmType::FCFS;
            }
            else {
                std::cerr << "Error: Unknown 'scheduler' type in config.txt: " << schedulerTypeStr << std::endl;
                std::cerr << "Supported types: FCFS" << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            if (algoType == SchedulerAlgorithmType::NONE) {
                std::cerr << "Error: Scheduler algorithm not correctly determined from config." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            int batchProcessFreq = 0;
            auto it_freq = config.find("batch-process-freq");
            if (it_freq != config.end()) {
                try {
                    batchProcessFreq = std::stoi(it_freq->second);
                    if (batchProcessFreq <= 0) {
                        std::cerr << "Warning: 'batch-process-freq' must be a positive integer. Auto-generation disabled." << std::endl;
                        batchProcessFreq = 0;
                    }
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Warning: Invalid 'batch-process-freq' value. Auto-generation disabled." << std::endl;
                    batchProcessFreq = 0;
                } catch (const std::out_of_range& e) {
                    std::cerr << "Warning: 'batch-process-freq' value out of range. Auto-generation disabled." << std::endl;
                    batchProcessFreq = 0;
                }
            } else {
                std::cout << "Info: 'batch-process-freq' not found in config.txt. Auto-generation disabled." << std::endl;
            }

            ConsoleManager::getInstance()->initializeSystem(numCpus, algoType, batchProcessFreq);
            initialized = true;
        } else {
            std::cout << "Please type 'initialize' first before using other commands." << std::endl;
        }
    } else {
        handleMainCommands(command);
    }
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
        std::cout << "Console is already initialized." << std::endl;
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

            bool created = ConsoleManager::getInstance()->createProcessConsole(processName);
            if (created) {
                ConsoleManager::getInstance()->switchToProcessConsole(processName);
            }
        }
    } else if (std::regex_match(command, match, screen_ls_regex)) {
        auto consoleManager = ConsoleManager::getInstance();
        Scheduler* scheduler = consoleManager->getScheduler();
        auto allProcesses = ConsoleManager::getInstance()->getAllProcesses();

        std::vector<Process> activeProcesses;
        std::vector<Process> finishedProcesses;

        for (const auto& pair : allProcesses) {
            const Process& p = pair.second;
            if (p.getStatus() == ProcessStatus::FINISHED) {
                finishedProcesses.push_back(p);
            } else {
                activeProcesses.push_back(p);
            }
        }
        
        std::cout << "\n--- Scheduler Status ---" << std::endl;
        if (scheduler && scheduler->isRunning()) {
            double cpuUtilization = scheduler->getCpuUtilization();
            int coresUsed = scheduler->getCoresUsed();            
            int coresAvailable = scheduler->getCoresAvailable();  
            int totalCores = scheduler->getTotalCores();           

            std::cout << " Total Cores: " << totalCores << std::endl;
            std::cout << " Cores Used: " << coresUsed << std::endl;
            std::cout << " Cores Available: " << coresAvailable << std::endl;
            std::cout << " CPU Utilization: " << std::fixed << std::setprecision(2) << cpuUtilization << "%" << std::endl;
        } else {
            std::cout << " Scheduler is not running. Use 'scheduler-start' to activate." << std::endl;
        }

        std::cout << "\n--- Active Processes ---" << std::endl;
        if (activeProcesses.empty()) {
            std::cout << " No active processes found." << std::endl;
        } else {
            for (const auto& p : activeProcesses) {
                std::string statusStr;
                switch (p.getStatus()) {
                    case ProcessStatus::NEW: statusStr = "NEW"; break;
                    case ProcessStatus::IDLE: statusStr = "IDLE"; break;
                    case ProcessStatus::RUNNING: statusStr = "RUNNING"; break;
                    case ProcessStatus::PAUSED: statusStr = "PAUSED"; break;
                    case ProcessStatus::FINISHED: statusStr = "FINISHED"; break; 
                }
                std::cout << " " << p.getProcessName() 
                          << " (" << p.getCreationTime() << ") "
                          << "Status: " << statusStr
                          << " Core: " << (p.getCpuCoreExecuting() == -1 ? "N/A" : std::to_string(p.getCpuCoreExecuting()))
                          << " " << p.getCurrentCommandIndex() << "/" << p.getTotalInstructionLines() 
                          << std::endl;
            }
        }

        std::cout << "\n--- Finished Processes ---" << std::endl;
        if (finishedProcesses.empty()) {
            std::cout << " No finished processes found." << std::endl;
        } else {
            for (const auto& p : finishedProcesses) {
                std::cout << " " << p.getProcessName() 
                          << " (" << p.getCreationTime() << ") "
                          << "Status: " << "FINISHED" // Status is already FINISHED for these
                          << " Core: " << (p.getCpuCoreExecuting() == -1 ? "N/A" : std::to_string(p.getCpuCoreExecuting()))
                          << " " << p.getCurrentCommandIndex() << "/" << p.getTotalInstructionLines() 
                          << std::endl;
            }
        }
        std::cout << std::flush; 
    } else if (command == "scheduler-test") {
        std::cout << "'scheduler-test' command recognized. Doing something." << std::endl;
    } else if (command == "scheduler-start") {
        ConsoleManager::getInstance()->startScheduler();
        std::cout << "Scheduler started." << std::endl;
    }else if (command == "scheduler-stop") {
        ConsoleManager::getInstance()->stopScheduler();
        std::cout << "Scheduler stopped." << std::endl;
    } else if (command == "report-util") {
        std::cout << "'report-util' command recognized. Doing something." << std::endl;
    } else if (command == "clear") {
        onEnabled(); 
    } else {
        std::cout << "Unknown command: " << command << std::endl;
    }
}