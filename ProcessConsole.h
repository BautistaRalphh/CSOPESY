#ifndef PROCESS_CONSOLE_H
#define PROCESS_CONSOLE_H

#include "AConsole.h"
#include "ConsoleManager.h" 

class ProcessConsole : public AConsole { 
private:
    Process currentProcessData;

    void displayProcessInfo();

public:
    ProcessConsole(const Process& processData);

    void onEnabled() override;
    void display() override;
    //void process() override;
    void handleCommand(const std::string& command) override;
};

#endif