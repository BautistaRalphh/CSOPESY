#pragma once

#include "AConsole.h"
#include "Process.h"       

class Process;

class ProcessConsole : public AConsole {
private:
    Process* currentProcessData;

    void displayProcessInfo();

public:
    ProcessConsole(Process* processData); 
    void onEnabled() override; 
    void display() override;  
    void handleCommand(const std::string& command) override; 

    void updateProcessData(Process* newData); 
};