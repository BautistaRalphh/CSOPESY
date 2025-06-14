#include "Process.h"

#include <iostream>

Process::Process(const std::string& name, const std::string& p_id, const std::string& c_time)
    : processName(name),
      pid(p_id),
      creationTime(c_time),
      currentCommandIndex(0),
      totalInstructionLines(0),
      status(ProcessStatus::IDLE),
      cpuCoreExecuting(-1), 
      finishTime("N/A") {
}

void Process::addCommand(const std::string& rawCommand) {
    std::stringstream ss(rawCommand);
    std::string commandTypeStr;
    ss >> commandTypeStr;

    CommandType type = CommandType::UNKNOWN;
    std::vector<std::string> args;
    std::string arg;

    for (char &c : commandTypeStr) c = toupper(c);

    if (commandTypeStr == "PRINT") {
        type = CommandType::PRINT;
        std::string message;
        std::getline(ss, message);
        if (!message.empty() && message[0] == ' ') {
            message = message.substr(1);
        }
        args.push_back(message);
    }
    // Future: Implement parsing for DECLARE, ADD, SUBTRACT, SLEEP, FOR
    // else if (commandTypeStr == "DECLARE") { ... }
    // else if (commandTypeStr == "ADD") { ... }
    // ...

    commands.emplace_back(type, args);
    totalInstructionLines = commands.size();
}


void Process::generateDummyPrintCommands(int count, const std::string& baseMessage) {
    for (int i = 0; i < count; ++i) {
        std::ostringstream cmd;
        // Example: "print Instruction N for ProcessName"
        cmd << "PRINT " << baseMessage << " " << (i + 1);
        addCommand(cmd.str());
    }
}

const ParsedCommand* Process::getNextCommand() const {
    if (currentCommandIndex < commands.size()) {
        return &commands[currentCommandIndex];
    }
    return nullptr;
}

const ParsedCommand* Process::getCommandAtIndex(int index) const {
    if (index >= 0 && index < commands.size()) {
        return &commands[index];
    }
    return nullptr;
}

int Process::getCpuCoreExecuting() const {
    return cpuCoreExecuting;
}

void Process::setCpuCoreExecuting(int core) {
    cpuCoreExecuting = core;
}

void Process::declareVariable(const std::string& varName, uint16_t value) {
    variables[varName] = value; // Creates or updates the variable
}

bool Process::getVariableValue(const std::string& varName, uint16_t& value) const {
    auto it = variables.find(varName);
    if (it != variables.end()) {
        value = it->second;
        return true;
    }
    return false; 
}

void Process::setVariableValue(const std::string& varName, uint16_t value) {
    variables[varName] = value;
}

std::string Process::getCreationTime() const {
    return creationTime;
}

bool Process::doesVariableExist(const std::string& varName) const {
    return variables.count(varName) > 0;
}