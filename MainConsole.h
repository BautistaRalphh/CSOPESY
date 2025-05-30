#ifndef MAIN_CONSOLE_SCREEN_H
#define MAIN_CONSOLE_SCREEN_H

#include "AConsole.h"
#include <string>

class MainConsole : public AConsole {
public:
    MainConsole();

    void onEnabled() override;
    void display() override;
    void process(const std::string& command) override;

private:
    void displayHeader();
    void handleMainCommands(const std::string& command);

    bool headerDisplayed;
};

#endif 