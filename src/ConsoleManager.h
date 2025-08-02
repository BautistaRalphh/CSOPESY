#pragma once

#include "console/ProcessConsole.h"
#include "console/MainConsole.h"
#include "core/Process.h"
#include "core/Scheduler.h"
#include "memory/IMemoryAllocator.h"
#include "memory/FlatMemoryAllocator.h"
#include "memory/DemandPagingAllocator.h"
#include "memory/BackingStore.h"

#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <regex>
#include <memory>
#include <atomic> 
#include <thread>

class AConsole;
class ProcessConsole;
class Scheduler;

class ConsoleManager {
private:
    std::queue<std::shared_ptr<Process>> pendingProcesses;
    static ConsoleManager* instance;
    ConsoleManager();
    ConsoleManager(const ConsoleManager&) = delete; 
    ConsoleManager& operator=(const ConsoleManager&) = delete; 

    AConsole* activeConsole;

    bool exitApp;

    std::map<std::string, std::shared_ptr<Process>> processes;
    std::map<std::string, std::shared_ptr<Process>> finishedProcesses;

public:
    const std::queue<std::shared_ptr<Process>>& getPendingProcesses() const { return pendingProcesses; }
    const std::map<std::string, std::shared_ptr<Process>>& getFinishedProcesses() const { return finishedProcesses; }

    std::map<std::string, std::unique_ptr<ProcessConsole>> processConsoleScreens;

    std::string getTimestamp();

    std::unique_ptr<Scheduler> scheduler;
    std::atomic<bool> schedulerStarted{false}; 

    int batchProcessFrequency;
    std::unique_ptr<std::thread> batchGenThread;
    std::atomic<bool> batchGenRunning;         
    long long nextBatchTickTarget;            

    //std::atomic<long long>* cpuCyclesPtr = nullptr; 
    uint32_t minInstructionsPerProcess;
    uint32_t maxInstructionsPerProcess;
    uint32_t processDelayPerExecution;
    uint32_t quantumCycles;
    uint32_t maxOverallMemory;
    uint32_t memoryPerFrame;
    uint32_t minMemoryPerProcess;
    uint32_t maxMemoryPerProcess;

    std::unique_ptr<DemandPagingAllocator> memoryAllocator;

public:
    IMemoryAllocator* getMemoryAllocator() const { return memoryAllocator.get(); }
    void createBatchProcess();   
    void batchGenLoop();        
    void startBatchGen();
    
public:
    static ConsoleManager* getInstance(); 
    static void cleanupInstance();

    bool readConfigFile(const std::string& filename, std::map<std::string, std::string>& config);
    void setActiveConsole(AConsole* console);
    void handleCommand(const std::string& command);
    void drawConsole();
    void setExitApp(bool val);
    bool getExitApp() const; 

    void initializeSystem(
    int numCpus,
    SchedulerAlgorithmType algoType,
    int batchProcessFreq,
    uint32_t minIns,
    uint32_t maxIns,
    uint32_t delaysPerExec,
    uint32_t quantumCyc,
    uint32_t maxOverallMem,
    uint32_t memPerFrame,
    uint32_t minMemPerProc,
    uint32_t maxMemPerProc
    );

    bool createProcessConsole(const std::string& name);
    bool createCustomProcessConsole(const std::string& name, const std::vector<std::string>& instructions);
    void switchToProcessConsole(const std::string& name);
    void cleanupTerminatedProcessConsole(const std::string& name);
    bool doesProcessExist(const std::string& name) const;
    std::shared_ptr<const Process> getProcess(const std::string& name) const;
    std::shared_ptr<Process> getProcessMutable(const std::string& name);
    const std::map<std::string, std::shared_ptr<Process>>& getAllProcesses() const;
    std::vector<std::shared_ptr<Process>> getProcesses() const;

    std::unique_ptr<MainConsole> mainConsole;
    AConsole* getMainConsole() const;

    void startScheduler();
    void stopScheduler(); 
    Scheduler* getScheduler() const;

    void setCpuCyclesCounter(std::atomic<long long>* counterPtr);
    void stopBatchGen();

    uint32_t getMinInstructionsPerProcess() const { return minInstructionsPerProcess; }
    uint32_t getMaxInstructionsPerProcess() const { return maxInstructionsPerProcess; }
};