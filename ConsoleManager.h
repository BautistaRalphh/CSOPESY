#pragma once

#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <regex>
#include <memory>
#include <atomic> 

#include "AConsole.h"
#include "MainConsole.h"
#include "Process.h" 
#include "Scheduler.h"

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

    // std::unique_ptr<Scheduler> scheduler; // ALLEN AND JORENIE PART
    std::unique_ptr<Scheduler> scheduler;

public:
    static ConsoleManager* getInstance(); 

    void setActiveConsole(AConsole* console);
    void handleCommand(const std::string& command);
    void drawConsole();
    void setExitApp(bool val);
    bool applicationExit() const;

    void createProcessConsole(const std::string& name);
    void switchToProcessConsole(const std::string& name);
    bool doesProcessExist(const std::string& name) const;
    const Process* getProcess(const std::string& name) const;

    std::map<std::string, Process> getAllProcesses() const; 

    std::unique_ptr<MainConsole> mainConsole;
    AConsole* getMainConsole() const;
};