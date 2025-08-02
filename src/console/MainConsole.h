#pragma once

#include "AConsole.h"
#include <string>
#include <vector>

class MainConsole : public AConsole {
public:
    MainConsole();

    void onEnabled() override;
    void display() override;
    //void process() override;
    void handleCommand(const std::string& command) override;

private:
    void displayHeader();
    void handleMainCommands(const std::string& command);
    std::vector<std::string> parseInstructions(const std::string& instructionsStr);

    bool headerDisplayed;
    bool initialized;
};