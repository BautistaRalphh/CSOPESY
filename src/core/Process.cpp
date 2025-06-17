#include "Process.h"

#include <iostream>


/* will be removed after h6 */
#include <fstream>
/* will be removed after h6 */
#include <filesystem>

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

        std::string restOfLine;
        std::getline(ss, restOfLine);
        if (!restOfLine.empty() && restOfLine[0] == ' ') {
            restOfLine = restOfLine.substr(1);
        }

        args.push_back(restOfLine);  // Store full message as one string
    }
    else if (commandTypeStr == "DECLARE") {
        type = CommandType::DECLARE;
        std::string var;
        uint16_t val;
        ss >> var >> val;
        args.push_back(var);
        args.push_back(std::to_string(val));
    }
    else if (commandTypeStr == "ADD") {
        type = CommandType::ADD;
        std::string var1, var2, var3;
        ss >> var1 >> var2 >> var3;
        args = { var1, var2, var3 };
    }
    else if (commandTypeStr == "SUBTRACT") {
        type = CommandType::SUBTRACT;
        std::string var1, var2, var3;
        ss >> var1 >> var2 >> var3;
        args = { var1, var2, var3 };
    }
    else if (commandTypeStr == "SLEEP") {
        type = CommandType::SLEEP;
        std::string ticks;
        ss >> ticks;
        args.push_back(ticks);
    }
    // Future: Implement parsing for DECLARE, ADD, SUBTRACT, SLEEP, FOR
    // else if (commandTypeStr == "DECLARE") { ... }
    // else if (commandTypeStr == "ADD") { ... }
    // ...

    commands.emplace_back(type, args);
    totalInstructionLines = commands.size();
}


/*to be removed*/
void Process::generateDummyPrintCommands(int count, const std::string& baseMessage) {
    for (int i = 0; i < count; ++i) {
        std::ostringstream cmd;
        // Example: "print Instruction N for ProcessName"
        cmd << "PRINT " << baseMessage;
        addCommand(cmd.str());
    }
}

void Process::generateDummyCommands(int count) {
    addCommand("SLEEP 20"); // to be fixed: 1st command dupe bug
    addCommand("DECLARE A 10");
    addCommand("DECLARE B 5");
    addCommand("DECLARE X 100");

    addCommand("ADD C A B"); 
    addCommand("SUBTRACT Y A X");

    addCommand("PRINT Starting dummy program...");
    addCommand("PRINT A is (A)");
    addCommand("PRINT B is (B)");
    addCommand("PRINT C is (C)");
    addCommand("PRINT D is (X)");
    addCommand("PRINT Y is (Y) testtttttttttttttt");

    addCommand("SLEEP 20");

    addCommand("PRINT Dummy program complete.");
}

/* will be removed after h6 */
void Process::writeToTextFile() const {
    std::filesystem::create_directories("process_logs");
    std::ofstream out("process_logs/" + getProcessName() + ".txt");
    if (!out.is_open()) return;

    out << "Process name: " << getProcessName() << "\n";
    out << "Logs: \n\n";

    for (auto& log : getLogEntries()) {
        out << log << "\n";
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

void Process::addLogEntry(const std::string& log) {
    executionLog.push_back(log);
}

const std::vector<std::string>& Process::getLogEntries() const {
    return executionLog;
}

bool Process::doesVariableExist(const std::string& varName) const {
    return variables.count(varName) > 0;
}