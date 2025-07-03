#include "MainConsole.h"
#include "ConsoleManager.h"
#include "Process.h"      
#include "core/Scheduler.h"
#include "core/Process.h"

#include <regex>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <vector> 
#include <fstream>
#include <ctime>
#include <sstream>

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
                if (numCpus <= 0) throw std::invalid_argument("non-positive");
            } catch (...) {
                std::cerr << "Error: Invalid or missing 'num-cpu' in config.txt." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            std::string schedulerTypeStr = config["scheduler"];
            SchedulerAlgorithmType algoType = SchedulerAlgorithmType::NONE;
            if (schedulerTypeStr == "fcfs") algoType = SchedulerAlgorithmType::fcfs;
            else if (schedulerTypeStr == "rr") algoType = SchedulerAlgorithmType::rr;
            else {
                std::cerr << "Error: Unknown 'scheduler' type in config.txt: " << schedulerTypeStr << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            int batchProcessFreq = 0;
            try {
                batchProcessFreq = std::stoi(config["batch-process-freq"]);
                if (batchProcessFreq <= 0) batchProcessFreq = 0;
            } catch (...) {
                batchProcessFreq = 0;
            }

            uint32_t minIns = 0, maxIns = 0, delaysPerExec = 0, quantumCycles = 0;

            try { minIns = std::stoul(config.at("min-ins")); } 
            catch (...) {
                std::cerr << "Error: Invalid or missing 'min-ins'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            try { maxIns = std::stoul(config.at("max-ins")); }
            catch (...) {
                std::cerr << "Error: Invalid or missing 'max-ins'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            if (minIns > maxIns) {
                std::cerr << "Error: 'min-ins' > 'max-ins'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            try { delaysPerExec = std::stoul(config.at("delays-per-exec")); }
            catch (...) {
                std::cerr << "Error: Invalid or missing 'delays-per-exec'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            try {
                quantumCycles = std::stoul(config.at("quantum-cycles"));
                if (quantumCycles <= 0) throw std::invalid_argument("non-positive");
            } catch (...) {
                std::cerr << "Error: Invalid or missing 'quantum-cycles'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            // Memory-related 
            uint32_t maxOverallMem = 0, memPerFrame = 0, minMemPerProc = 0, maxMemPerProc = 0;

            try {
                maxOverallMem = std::stoul(config.at("max-overall-mem"));
                if (maxOverallMem == 0) throw std::invalid_argument("zero");
            } catch (...) {
                std::cerr << "Error: Invalid or missing 'max-overall-mem'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            try {
                memPerFrame = std::stoul(config.at("mem-per-frame"));
                if (memPerFrame == 0) throw std::invalid_argument("zero");
            } catch (...) {
                std::cerr << "Error: Invalid or missing 'mem-per-frame'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            try {
                minMemPerProc = std::stoul(config.at("min-mem-per-proc"));
            } catch (...) {
                std::cerr << "Error: Invalid or missing 'min-mem-per-proc'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            try {
                maxMemPerProc = std::stoul(config.at("max-mem-per-proc"));
            } catch (...) {
                std::cerr << "Error: Invalid or missing 'max-mem-per-proc'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            if (minMemPerProc > maxMemPerProc) {
                std::cerr << "Error: 'min-mem-per-proc' cannot be greater than 'max-mem-per-proc'." << std::endl;
                std::cout << "Initialization failed." << std::endl;
                return;
            }

            ConsoleManager::getInstance()->initializeSystem(
                numCpus, algoType, batchProcessFreq,
                minIns, maxIns, delaysPerExec, quantumCycles
                , maxOverallMem, memPerFrame, minMemPerProc, maxMemPerProc
            );

            initialized = true;
        } else {
            std::cout << "Please type 'initialize' first before using other commands." << std::endl;
        }
    } else {
        handleMainCommands(command);
    }
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
            bool created = ConsoleManager::getInstance()->createProcessConsole(processName);
            if (created) {
                ConsoleManager::getInstance()->switchToProcessConsole(processName);
            }
        }
    } else if (std::regex_match(command, match, screen_ls_regex)) {
        std::ostringstream oss;
        std::streambuf* oldCout = std::cout.rdbuf();
        std::cout.rdbuf(oss.rdbuf());

        auto consoleManager = ConsoleManager::getInstance();
        Scheduler* scheduler = consoleManager->getScheduler();
        
       const auto& allProcesses = ConsoleManager::getInstance()->getProcesses();

        std::vector<std::shared_ptr<Process>> activeProcesses;
        std::vector<std::shared_ptr<Process>> finishedProcesses;

        for (const auto& procPtr : allProcesses) {
            if (procPtr->getStatus() == ProcessStatus::TERMINATED)
                finishedProcesses.push_back(procPtr);
            else
                activeProcesses.push_back(procPtr);
        }

        std::sort(activeProcesses.begin(), activeProcesses.end(), [](const auto& a, const auto& b) {
            return a->getCreationTime() < b->getCreationTime();
        });
        std::sort(finishedProcesses.begin(), finishedProcesses.end(), [](const auto& a, const auto& b) {
            return a->getCreationTime() < b->getCreationTime();
        });

        std::cout << "\n--- Scheduler Status ---" << std::endl;
        if (scheduler) {
            int totalCores = scheduler->getTotalCores();
            int coresUsed = scheduler->getCoresUsed();
            int coresAvailable = scheduler->getCoresAvailable();
            double cpuUtilization = scheduler->getCpuUtilization();

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
            for (const auto& p_ptr : activeProcesses) { 
                std::string statusStr;
                switch (p_ptr->getStatus()) { 
                    case ProcessStatus::NEW: statusStr = "NEW"; break;
                    case ProcessStatus::READY: statusStr = "READY"; break;
                    case ProcessStatus::RUNNING: statusStr = "RUNNING"; break;
                    case ProcessStatus::PAUSED: statusStr = "PAUSED"; break; 
                    case ProcessStatus::TERMINATED: statusStr = "TERMINATED"; break; 
                    default: statusStr = "UNKNOWN"; break;
                }
                std::cout << " " << p_ptr->getProcessName() 
                                << " (" << p_ptr->getCreationTime() << ") "
                                << "Status: " << statusStr
                                << " Core: " << (p_ptr->getCpuCoreExecuting() == -1 ? "N/A" : std::to_string(p_ptr->getCpuCoreExecuting()))
                                << " " << p_ptr->getCurrentCommandIndex() << "/" << p_ptr->getTotalInstructionLines() 
                                << std::endl;
            }
        }

        std::cout << "\n--- Finished Processes ---" << std::endl;
        if (finishedProcesses.empty()) {
            std::cout << " No finished processes found." << std::endl;
        } else {
            for (const auto& p_ptr : finishedProcesses) { 
                std::cout << " " << p_ptr->getProcessName() 
                                << " (" << p_ptr->getCreationTime() << ") "
                                << "Status: " << "TERMINATED" 
                                << " Core: " << (p_ptr->getCpuCoreExecuting() == -1 ? "N/A" : std::to_string(p_ptr->getCpuCoreExecuting()))
                                << " " << p_ptr->getCurrentCommandIndex() << "/" << p_ptr->getTotalInstructionLines() 
                                << std::endl;
            }
        }
        std::cout.rdbuf(oldCout);
        std::cout << oss.str();
    } else if (command == "scheduler-start") {
        ConsoleManager::getInstance()->startScheduler();
        std::cout << "Scheduler started." << std::endl;
    } else if (command == "scheduler-stop") {
        ConsoleManager::getInstance()->stopBatchGen();
        std::cout << "Scheduler stopped." << std::endl;
    } else if (command == "report-util") {
        std::filesystem::path reportsDirPath = "reports";
        if (!std::filesystem::exists(reportsDirPath)) {
            try {
                std::filesystem::create_directory(reportsDirPath);
                std::cout << "Created directory: " << reportsDirPath << std::endl;
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error creating directory 'reports': " << e.what() << std::endl;
                std::cout << "Failed to generate report." << std::endl;
                return;
            }
        }

        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm* localTime = std::localtime(&currentTime);

        std::ostringstream filenameStream;
        filenameStream << "reports/scheduler_report_";
        filenameStream << std::put_time(localTime, "%Y-%m-%d_%H-%M-%S") << ".txt";
        std::string filename = filenameStream.str();

        std::ofstream reportFile(filename);
        if (!reportFile.is_open()) {
            std::cerr << "Error: Could not create report file: " << filename << std::endl;
            return;
        }

        std::ostringstream capturedOutput;
        std::streambuf* oldCoutBuffer = std::cout.rdbuf();
        std::cout.rdbuf(capturedOutput.rdbuf());

        auto consoleManager = ConsoleManager::getInstance();
        Scheduler* scheduler = consoleManager->getScheduler();
        const auto& allProcesses = ConsoleManager::getInstance()->getProcesses();

        std::vector<std::shared_ptr<Process>> activeProcesses;
        std::vector<std::shared_ptr<Process>> finishedProcesses;

        for (const auto& procPtr : allProcesses) {
            if (procPtr->getStatus() == ProcessStatus::TERMINATED)
                finishedProcesses.push_back(procPtr);
            else
                activeProcesses.push_back(procPtr);
        }

        std::cout << "\n--- Scheduler Status ---" << std::endl;
        if (scheduler && scheduler->isRunning()) {
            int totalCores = scheduler->getTotalCores();
            int coresUsed = scheduler->getCoresUsed();
            int coresAvailable = scheduler->getCoresAvailable();
            double cpuUtilization = scheduler->getCpuUtilization();

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
            for (const auto& p_ptr : activeProcesses) { 
                std::string statusStr;
                switch (p_ptr->getStatus()) { 
                    case ProcessStatus::NEW: statusStr = "NEW"; break;
                    case ProcessStatus::READY: statusStr = "READY"; break;
                    case ProcessStatus::RUNNING: statusStr = "RUNNING"; break;
                    case ProcessStatus::PAUSED: statusStr = "PAUSED"; break; 
                    case ProcessStatus::TERMINATED: statusStr = "TERMINATED"; break; 
                    default: statusStr = "UNKNOWN"; break;
                }
                std::cout << " " << p_ptr->getProcessName() 
                                << " (" << p_ptr->getCreationTime() << ") "
                                << "Status: " << statusStr
                                << " Core: " << (p_ptr->getCpuCoreExecuting() == -1 ? "N/A" : std::to_string(p_ptr->getCpuCoreExecuting()))
                                << " " << p_ptr->getCurrentCommandIndex() << "/" << p_ptr->getTotalInstructionLines() 
                                << std::endl;
            }
        }

        std::cout << "\n--- Finished Processes ---" << std::endl;
        if (finishedProcesses.empty()) {
            std::cout << " No finished processes found." << std::endl;
        } else {
            for (const auto& p_ptr : finishedProcesses) { 
                std::cout << " " << p_ptr->getProcessName() 
                                << " (" << p_ptr->getCreationTime() << ") "
                                << "Status: " << "TERMINATED" 
                                << " Core: " << (p_ptr->getCpuCoreExecuting() == -1 ? "N/A" : std::to_string(p_ptr->getCpuCoreExecuting()))
                                << " " << p_ptr->getCurrentCommandIndex() << "/" << p_ptr->getTotalInstructionLines() 
                                << std::endl;
            }
        }

        std::cout.rdbuf(oldCoutBuffer);
        
        reportFile << capturedOutput.str();
        reportFile.close();
        std::cout << "Scheduler report saved to " << filename << std::endl;
    } else if (command == "clear") {
        onEnabled(); 
    } else {
        std::cout << "Unknown command: " << command << std::endl;
    }
}