#include "MainConsole.h"
#include "ConsoleManager.h"
#include "Process.h"      
#include "core/Scheduler.h"
#include "core/Process.h"
#include "memory/DemandPagingAllocator.h"
#include "memory/BackingStore.h"

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

            std::ofstream ofs("csopesy-backing-store.txt", std::ofstream::out | std::ofstream::trunc);
            ofs.close();

            initialized = true;
        } else {
            std::cout << "Please type 'initialize' first before using other commands." << std::endl;
        }
    } else {
        handleMainCommands(command);
    }
}

void MainConsole::handleMainCommands(const std::string& command) {
    std::regex screen_cmd_regex(R"(^screen\s+(-r)\s+(\w+)$)");
    std::regex screen_s_regex(R"(^screen\s+-s\s+(\w+)(?:\s+(\d+))?$)");
    std::regex screen_custom_regex(R"(^screen\s+-c\s+(\w+)(?:\s+(\d+))?\s+\"(.+)\"$)");
    std::regex screen_ls_regex(R"(^screen\s+-ls$)");
    std::smatch match;

    if (command == "initialize") {
        std::cout << "Console is already initialized." << std::endl;
    } else if (std::regex_match(command, match, screen_custom_regex)) {
        std::string processName = match[1].str();
        std::string memorySizeStr = match[2].str(); 
        std::string instructionsStr = match[3].str();
        
        std::vector<std::string> instructions = parseInstructions(instructionsStr);
        if (instructions.empty()) {
            std::cout << "Error: No valid instructions provided." << std::endl;
            return;
        }
        
        if (instructions.size() < 1 || instructions.size() > 50) {
            std::cout << "Error: Invalid command. Instruction count must be between 1 and 50." << std::endl;
            return;
        }
        
        uint32_t memorySize = 0;
        
        if (memorySizeStr.empty()) {
            auto consoleManager = ConsoleManager::getInstance();
            memorySize = consoleManager->minMemoryPerProcess;
        } else {
            try {
                memorySize = std::stoul(memorySizeStr);
                if (memorySize < 64) {
                    std::cout << "Error: Memory size must be at least 64 bytes." << std::endl;
                    return;
                }
            } catch (...) {
                std::cout << "Error: Invalid memory size specified." << std::endl;
                return;
            }
        }
        
        bool created = ConsoleManager::getInstance()->createCustomProcessConsole(processName, instructions, memorySize);
        if (created) {
            ConsoleManager::getInstance()->switchToProcessConsole(processName);
        }
    } else if (std::regex_match(command, match, screen_cmd_regex)) {
        std::string option = match[1].str(); 
        std::string processName = match[2].str();

        if (option == "-r") {
            ConsoleManager::getInstance()->switchToProcessConsole(processName);
        }
    } else if (std::regex_match(command, match, screen_s_regex)) {
        std::string processName = match[1].str();
        std::string memorySizeStr = match[2].str(); 
        
        uint32_t memorySize = 0;
        
        if (memorySizeStr.empty()) {
            auto consoleManager = ConsoleManager::getInstance();
            memorySize = consoleManager->minMemoryPerProcess;
        } else {
            try {
                memorySize = std::stoul(memorySizeStr);
                if (memorySize < 64) {
                    std::cout << "Error: Memory size must be at least 64 bytes." << std::endl;
                    return;
                }
            } catch (...) {
                std::cout << "Error: Invalid memory size specified." << std::endl;
                return;
            }
        }
        
        bool created = ConsoleManager::getInstance()->createProcessConsole(processName, memorySize);
        if (created) {
            ConsoleManager::getInstance()->switchToProcessConsole(processName);
        }
    } else if (std::regex_match(command, match, screen_ls_regex)) {
        std::ostringstream oss;
        std::streambuf* oldCout = std::cout.rdbuf();
        std::cout.rdbuf(oss.rdbuf());

        auto consoleManager = ConsoleManager::getInstance();
        Scheduler* scheduler = consoleManager->getScheduler();
        
        const auto& allProcesses = ConsoleManager::getInstance()->getProcesses();
        std::vector<std::shared_ptr<Process>> activeProcesses;
        for (const auto& procPtr : allProcesses) {
            if (procPtr->getStatus() != ProcessStatus::TERMINATED)
                activeProcesses.push_back(procPtr);
        }

        const auto& pendingQueue = ConsoleManager::getInstance()->getPendingProcesses();
        std::queue<std::shared_ptr<Process>> pendingCopy = pendingQueue;
        while (!pendingCopy.empty()) {
            activeProcesses.push_back(pendingCopy.front());
            pendingCopy.pop();
        }
        std::sort(activeProcesses.begin(), activeProcesses.end(), [](const auto& a, const auto& b) {
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

        const auto& finishedMap = ConsoleManager::getInstance()->getFinishedProcesses();
        std::vector<std::shared_ptr<Process>> finishedList;
        for (const auto& pair : finishedMap) {
            finishedList.push_back(pair.second);
        }
        std::sort(finishedList.begin(), finishedList.end(), [](const auto& a, const auto& b) {
            return a->getCreationTime() < b->getCreationTime();
        });

        std::cout << "\n--- Finished Processes ---" << std::endl;
        if (finishedList.empty()) {
            std::cout << " No finished processes found." << std::endl;
        } else {
            for (const auto& p_ptr : finishedList) { 
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
        std::cout << "Batch process generation stopped. Scheduler continues running." << std::endl;
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
        for (const auto& procPtr : allProcesses) {
            if (procPtr->getStatus() != ProcessStatus::TERMINATED)
                activeProcesses.push_back(procPtr);
        }
        std::sort(activeProcesses.begin(), activeProcesses.end(), [](const auto& a, const auto& b) {
            return a->getCreationTime() < b->getCreationTime();
        });

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

        const auto& finishedMap = ConsoleManager::getInstance()->getFinishedProcesses();
        std::vector<std::shared_ptr<Process>> finishedList;
        for (const auto& pair : finishedMap) {
            finishedList.push_back(pair.second);
        }
        std::sort(finishedList.begin(), finishedList.end(), [](const auto& a, const auto& b) {
            return a->getCreationTime() < b->getCreationTime();
        });

        std::cout << "\n--- Finished Processes ---" << std::endl;
        if (finishedList.empty()) {
            std::cout << " No finished processes found." << std::endl;
        } else {
            for (const auto& p_ptr : finishedList) { 
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
    } else if (command == "vmstat") {
        auto consoleManager = ConsoleManager::getInstance();
        auto memoryAllocator = consoleManager->getMemoryAllocator();
        auto scheduler = consoleManager->getScheduler();
        
        std::cout << "\n--- Virtual Memory Statistics ---" << std::endl;
        
        if (!memoryAllocator) {
            std::cout << " Memory allocator not initialized." << std::endl;
            return;
        }
        
        // Memory Statistics
        uint32_t totalMemory = consoleManager->maxOverallMemory;
        uint32_t frameSize = consoleManager->memoryPerFrame;
        uint32_t totalFrames = totalMemory / frameSize;
        
        std::vector<std::shared_ptr<Process>> allProcesses;
        
        for (const auto& procPtr : consoleManager->getProcesses()) {
            allProcesses.push_back(procPtr);
        }
        
        const auto& finishedMap = consoleManager->getFinishedProcesses();
        for (const auto& pair : finishedMap) {
            allProcesses.push_back(pair.second);
        }
        
        const auto& pendingQueue = consoleManager->getPendingProcesses();
        std::queue<std::shared_ptr<Process>> pendingCopy = pendingQueue;
        while (!pendingCopy.empty()) {
            allProcesses.push_back(pendingCopy.front());
            pendingCopy.pop();
        }
        
        uint32_t actualMemoryUsed = 0;
        uint32_t totalPagesAllocated = 0;
        uint32_t framesInUse = 0; // Count actual frames in physical memory
        
        for (const auto& process : allProcesses) {
            uint32_t pagesAlloc = process->getPagesAllocated();
            if (pagesAlloc > 0 && process->getStatus() != ProcessStatus::TERMINATED) {
                // Only count memory from non-terminated processes
                totalPagesAllocated += pagesAlloc;
                
                // Count frames actually in physical memory for memory calculation
                if (auto demandPagingAllocator = dynamic_cast<DemandPagingAllocator*>(memoryAllocator)) {
                    uint32_t pagesInMemory = demandPagingAllocator->getPagesInPhysicalMemory(process->getPid());
                    framesInUse += pagesInMemory;
                    actualMemoryUsed += pagesInMemory * frameSize; // Memory = pages in memory * frame size
                } else {
                    framesInUse += pagesAlloc; // For non-demand paging, all pages are in memory
                    actualMemoryUsed += process->getMemoryRequired();
                }
            }
        }
        
        uint32_t usedMemory = actualMemoryUsed;
        uint32_t freeMemory = totalMemory - usedMemory;
        
        long long totalCpuTicks = 0;
        long long activeCpuTicks = 0;
        long long idleCpuTicks = 0;
        
        if (scheduler) {
            totalCpuTicks = scheduler->getTotalCpuTicks();
            activeCpuTicks = scheduler->getActiveCpuTicks();
            idleCpuTicks = scheduler->getIdleCpuTicks();
        }
        
        long long pagesPagedIn = 0;
        long long pagesPagedOut = 0;
        
        if (auto demandPagingAllocator = dynamic_cast<DemandPagingAllocator*>(memoryAllocator)) {
            pagesPagedIn = demandPagingAllocator->getTotalPagesPagedIn();
            pagesPagedOut = demandPagingAllocator->getTotalPagesPagedOut();
        }
        
        std::cout << "\n--- Memory Information ---" << std::endl;
        std::cout << " Total Memory: " << totalMemory << " KB" << std::endl;
        std::cout << " Used Memory: " << usedMemory << " KB" << std::endl;
        std::cout << " Free Memory: " << freeMemory << " KB" << std::endl;
        std::cout << " Memory per Frame: " << frameSize << " KB" << std::endl;
        std::cout << " Memory Utilization: " << std::fixed << std::setprecision(2) 
                  << (static_cast<double>(usedMemory) / totalMemory * 100.0) << "%" << std::endl;
        
        std::cout << "\n--- CPU Information ---" << std::endl;
        std::cout << " Total CPU Ticks: " << totalCpuTicks << std::endl;
        std::cout << " Active CPU Ticks: " << activeCpuTicks << std::endl;
        std::cout << " Idle CPU Ticks: " << idleCpuTicks << std::endl;
        if (totalCpuTicks > 0) {
            std::cout << " CPU Utilization: " << std::fixed << std::setprecision(2) 
                      << (static_cast<double>(activeCpuTicks) / totalCpuTicks * 100.0) << "%" << std::endl;
        }
        
        std::cout << "\n--- Paging Information ---" << std::endl;
        std::cout << " Num Paged In: " << pagesPagedIn << std::endl;
        std::cout << " Num Paged Out: " << pagesPagedOut << std::endl;
        
    } else if (command == "process-smi") {
        auto consoleManager = ConsoleManager::getInstance();
        auto memoryAllocator = consoleManager->getMemoryAllocator();
        
        if (!memoryAllocator) {
            std::cout << " Memory allocator not initialized." << std::endl;
            return;
        }
        
        uint32_t totalMemory = consoleManager->maxOverallMemory;
        uint32_t frameSize = consoleManager->memoryPerFrame;
        uint32_t totalFrames = totalMemory / frameSize;
        
        std::vector<std::shared_ptr<Process>> allProcesses;
        
        for (const auto& procPtr : consoleManager->getProcesses()) {
            allProcesses.push_back(procPtr);
        }
        
        const auto& finishedMap = consoleManager->getFinishedProcesses();
        for (const auto& pair : finishedMap) {
            allProcesses.push_back(pair.second);
        }
        
        const auto& pendingQueue = consoleManager->getPendingProcesses();
        std::queue<std::shared_ptr<Process>> pendingCopy = pendingQueue;
        while (!pendingCopy.empty()) {
            allProcesses.push_back(pendingCopy.front());
            pendingCopy.pop();
        }
        
        uint32_t actualMemoryUsed = 0;
        uint32_t totalPagesAllocated = 0;
        uint32_t framesInUse = 0; 
        
        for (const auto& process : allProcesses) {
            uint32_t pagesAlloc = process->getPagesAllocated();
            if (pagesAlloc > 0 && process->getStatus() != ProcessStatus::TERMINATED) {
                totalPagesAllocated += pagesAlloc;
                
                if (auto demandPagingAllocator = dynamic_cast<DemandPagingAllocator*>(memoryAllocator)) {
                    uint32_t pagesInMemory = demandPagingAllocator->getPagesInPhysicalMemory(process->getPid());
                    framesInUse += pagesInMemory;
                    actualMemoryUsed += pagesInMemory * frameSize; 
                } else {
                    framesInUse += pagesAlloc;
                    actualMemoryUsed += process->getMemoryRequired();
                }
            }
        }
        
        uint32_t usedMemory = actualMemoryUsed;
        uint32_t freeMemory = totalMemory - usedMemory;
        
        long long pagesPagedIn = 0;
        long long pagesPagedOut = 0;
        
        if (auto demandPagingAllocator = dynamic_cast<DemandPagingAllocator*>(memoryAllocator)) {
            pagesPagedIn = demandPagingAllocator->getTotalPagesPagedIn();
            pagesPagedOut = demandPagingAllocator->getTotalPagesPagedOut();
        }
        
        std::cout << "\n--- Process Memory Usage ---" << std::endl;
        std::cout << std::left << std::setw(15) << "Process Name" 
                  << std::setw(12) << "PID"
                  << std::setw(12) << "Memory (KB)"
                  << std::setw(12) << "Pages Used"
                  << std::setw(15) << "Status"
                  << std::setw(15) << "Phys. Pages"
                  << std::setw(15) << "Store Pages"
                  << "Paging Activity" << std::endl;
        std::cout << std::string(120, '-') << std::endl;
        
        std::sort(allProcesses.begin(), allProcesses.end(), [](const auto& a, const auto& b) {
            return a->getCreationTime() < b->getCreationTime();
        });
        
        for (const auto& process : allProcesses) {
            std::string statusStr;
            switch (process->getStatus()) {
                case ProcessStatus::NEW: statusStr = "NEW"; break;
                case ProcessStatus::READY: statusStr = "READY"; break;
                case ProcessStatus::RUNNING: statusStr = "RUNNING"; break;
                case ProcessStatus::PAUSED: statusStr = "PAUSED"; break;
                case ProcessStatus::TERMINATED: statusStr = "TERMINATED"; break;
                default: statusStr = "UNKNOWN"; break;
            }
            
            uint32_t memoryReq = process->getMemoryRequired();
            uint32_t pagesAlloc = process->getPagesAllocated();
            
            int physicalPages = 0;
            int storePages = 0;
            if (auto demandPagingAllocator = dynamic_cast<DemandPagingAllocator*>(memoryAllocator)) {
                physicalPages = demandPagingAllocator->getPagesInPhysicalMemory(process->getPid());
                storePages = demandPagingAllocator->getPagesInBackingStore(process->getPid());
            }
            
            std::string pagingActivity = "None";
            if (pagesAlloc == 0) {
                if (process->getStatus() == ProcessStatus::TERMINATED) {
                    pagingActivity = "Deallocated";
                } else {
                    pagingActivity = "Pending";
                }
            } else {
                if (process->getStatus() == ProcessStatus::RUNNING) {
                    pagingActivity = "Active";
                } else if (process->getStatus() == ProcessStatus::READY || process->getStatus() == ProcessStatus::PAUSED) {
                    pagingActivity = "Idle";
                } else if (process->getStatus() == ProcessStatus::NEW) {
                    pagingActivity = "Allocated";
                } else if (process->getStatus() == ProcessStatus::TERMINATED) {
                    pagingActivity = "Deallocated";
                }
            }
            
            std::cout << std::left << std::setw(15) << process->getProcessName()
                      << std::setw(12) << process->getPid()
                      << std::setw(12) << memoryReq
                      << std::setw(12) << pagesAlloc
                      << std::setw(15) << statusStr
                      << std::setw(15) << physicalPages
                      << std::setw(15) << storePages
                      << pagingActivity << std::endl;
        }
        
        std::cout << std::string(120, '-') << std::endl;
        std::cout << " Total Pages Allocated: " << totalPagesAllocated << std::endl;
        std::cout << " Frames in Use: " << framesInUse << std::endl;
        std::cout << " Frame Utilization: " << framesInUse << "/" << totalFrames 
                  << " (" << std::fixed << std::setprecision(2) 
                  << (static_cast<double>(framesInUse) / totalFrames * 100.0) << "%)" << std::endl;
        
    } else if (command == "backing-store") {
        BackingStore::displayStatus();
    } else if (command == "clear") {
        onEnabled(); 
    } else {
        std::cout << "Unknown command: " << command << std::endl;
    }
}

std::vector<std::string> MainConsole::parseInstructions(const std::string& instructionsStr) {
    std::vector<std::string> instructions;
    std::string instruction;
    
    for (size_t i = 0; i < instructionsStr.length(); ++i) {
        char c = instructionsStr[i];
        
        if (c == ';') {
            instruction.erase(0, instruction.find_first_not_of(" \t\r\n"));
            instruction.erase(instruction.find_last_not_of(" \t\r\n") + 1);
            if (!instruction.empty()) {
                instructions.push_back(instruction);
            }
            instruction.clear();
        } else {
            instruction += c;
        }
    }
    
    instruction.erase(0, instruction.find_first_not_of(" \t\r\n"));
    instruction.erase(instruction.find_last_not_of(" \t\r\n") + 1);
    if (!instruction.empty()) {
        instructions.push_back(instruction);
    }
    
    return instructions;
}