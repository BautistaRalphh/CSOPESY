#pragma once

#include "AConsole.h"
#include "Process.h"       
#include <memory>

class Process;

class ProcessConsole : public AConsole {
private:
    std::shared_ptr<Process> currentProcessData;

    void displayProcessInfo();

public:
    ProcessConsole(std::shared_ptr<Process> processData);
    void onEnabled() override; 
    void display() override;  
    void handleCommand(const std::string& command) override; 

    void updateProcessData(std::shared_ptr<Process> newData);
};