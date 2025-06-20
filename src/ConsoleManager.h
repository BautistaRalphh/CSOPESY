#pragma once

#include "console/ProcessConsole.h"
#include "console/MainConsole.h"
#include "core/Process.h"
#include "core/Scheduler.h"

#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <regex>
#include <memory>
#include <atomic> 

class AConsole;
class ProcessConsole;
class Scheduler;

class ConsoleManager {
private:
    static ConsoleManager* instance;
    ConsoleManager();
    ConsoleManager(const ConsoleManager&) = delete; 
    ConsoleManager& operator=(const ConsoleManager&) = delete; 

    AConsole* activeConsole;

    bool exitApp;

    std::map<std::string, Process> processes;

    std::map<std::string, std::unique_ptr<ProcessConsole>> processConsoleScreens;

    std::string getTimestamp();

    std::unique_ptr<Scheduler> scheduler;
    std::atomic<bool> schedulerStarted{false}; 

    int batchProcessFrequency;
    std::unique_ptr<std::thread> batchGenThread;
    std::atomic<bool> batchGenRunning;         
    long long nextBatchTickTarget;            

    std::atomic<long long>* cpuCyclesPtr = nullptr; 

    void createBatchProcess();   
    void batchGenLoop();        
    void startBatchGen();
public:
    static ConsoleManager* getInstance(); 

    bool readConfigFile(const std::string& filename, std::map<std::string, std::string>& config);
    void setActiveConsole(AConsole* console);
    void handleCommand(const std::string& command);
    void drawConsole();
    void setExitApp(bool val);
    bool getExitApp() const; 

    void initializeSystem(int numCpus, SchedulerAlgorithmType type, int batchFreq);

    bool createProcessConsole(const std::string& name);
    void switchToProcessConsole(const std::string& name);
    bool doesProcessExist(const std::string& name) const;
    const Process* getProcess(const std::string& name) const;
    Process* getProcessMutable(const std::string& name);
    std::map<std::string, Process> getAllProcesses() const; 

    std::unique_ptr<MainConsole> mainConsole;
    AConsole* getMainConsole() const;

    void startScheduler();
    void stopScheduler(); 
    Scheduler* getScheduler() const;

    void setCpuCyclesCounter(std::atomic<long long>* counterPtr);
    void stopBatchGen();
};