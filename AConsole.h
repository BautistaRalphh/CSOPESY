#ifndef A_CONSOLE_H
#define A_CONSOLE_H

#include <string>
#include <iostream>

class ConsoleManager;

class AConsole {
protected:
    std::string name;

public:
    AConsole(const std::string& name);

    virtual ~AConsole() = default;
    std::string getName() const;
    virtual void onEnabled() = 0;
    virtual void display() = 0;
    //virtual void process() = 0;
    virtual void handleCommand(const std::string& command) = 0;

    friend class ConsoleManager;
};

#endif 

