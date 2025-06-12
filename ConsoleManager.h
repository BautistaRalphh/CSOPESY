#pragma once

#include "ConsoleManager.h"
#include "ProcessConsole.h" 
#include "MainConsole.h"    
#include "Process.h"       
#include "Scheduler.h"

#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <regex>
#include <memory>
#include <atomic> 

class ProcessConsole;

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
    bool schedulerStarted = false;

    bool readConfigFile(const std::string& filename, std::map<std::string, std::string>& config);
public:
    static ConsoleManager* getInstance(); 

    void setActiveConsole(AConsole* console);
    void handleCommand(const std::string& command);
    void drawConsole();
    void setExitApp(bool val);
    bool applicationExit() const;

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
    Scheduler* getScheduler();

};
