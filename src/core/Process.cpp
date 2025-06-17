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
    static int forDepth = 0;

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

        args.push_back(restOfLine);
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

    else if (commandTypeStr == "FOR") {
        if (forDepth >= 3) {
            std::cerr << "Too much nesting. Ignoring: " << rawCommand << std::endl;
            return;
        }

        type = CommandType::FOR;

        size_t openBracket = rawCommand.find('[');
        size_t closeBracket = rawCommand.rfind(']');

        std::string block = rawCommand.substr(openBracket + 1, closeBracket - openBracket - 1);
        std::string afterBlock = rawCommand.substr(closeBracket + 1);
        std::stringstream ss(afterBlock);
        int repeatCount = 1;
        ss >> repeatCount;

        std::vector<std::string> innerInstructions = splitInstructions(block);

        forDepth++;
        for (int i = 0; i < repeatCount; ++i) {
            for (const std::string& instr : innerInstructions) {
                addCommand(instr);
            }
        }
        forDepth--;

        return;
    }

    commands.emplace_back(type, args);
    totalInstructionLines = commands.size();
}

std::vector<std::string> Process::splitInstructions(const std::string& block) {
    std::vector<std::string> result;
    std::string current;
    int bracketDepth = 0;

    for (char c : block) {
        if (c == '[') bracketDepth++;
        if (c == ']') bracketDepth--;

        if (c == ';' && bracketDepth == 0) {
            result.push_back(trim(current));
            current.clear();
        } else {
            current += c;
        }
    }

    if (!current.empty()) result.push_back(trim(current));
    return result;
}

std::string Process::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    size_t last = str.find_last_not_of(" \t\r\n");
    return (first == std::string::npos) ? "" : str.substr(first, last - first + 1);
}

/*======================= COMMAND SYNTAX FORMAT =======================
-----------------------------------
Put inside addCommand("")
Example: addCommand("PRINT A");
-----------------------------------

PRINT - Format: "PRINT message"
- Prints the entire message string
Example: PRINT Hello, world!

DECLARE - Format: "DECLARE varName initialValue"
- Declares a variable with a starting value
Example: DECLARE x 10

ADD - Format: "ADD target source1 source2"
- target = source1 + source2
Example: ADD sum x y

SUBTRACT - Format: "SUBTRACT target source1 source2"
- target = source1 - source2
Example: SUBTRACT diff x y

SLEEP - Format: "SLEEP ticks"
- Pauses execution for the given number of ticks
Example: SLEEP 5

FOR - Format: "FOR [Instruction1; Instruction2; ...] RepeatCount"
- RepeatCount is optional; defaults to 1 if omitted
- Block can contain one or more instructions
Example 1: FOR [PRINT A; SLEEP 1] 2
Example 2: FOR [PRINT A]

=====================================================================*/
void Process::generateDummyCommands(int count) {
    for (int i = 0; i < count; ++i) {
        //to do: PUT RANDOMIZER HERE




        //FOR TESTING ONLY
        //============================================================
        addCommand("PRINT Hello world from <process_name>!"); // to do: make process_name appear | Print should appear when inside a proccess
        addCommand("DECLARE x 10");
        addCommand("DECLARE y 20");
        addCommand("ADD sum x y");
        addCommand("PRINT The sum is: (sum)");
        addCommand("SUBTRACT diff y x");
        addCommand("PRINT The difference is: (diff)");

        addCommand("SLEEP 10");

        // FOR loop with one instruction
        addCommand("FOR [PRINT Inside single-instruction loop] 2");

        // FOR loop with multiple instructions
        addCommand("FOR [PRINT IM SLEEPING FOR 20 TICKS; SLEEP 20] 2");

        // Nested FOR loop up to 3 levels
        addCommand("FOR [PRINT Level1; FOR [PRINT Level2; FOR [PRINT Level3] 2] 2] 2");

        addCommand("PRINT -----Loop Counter Test-----");
        addCommand("DECLARE counter 0");
        addCommand("DECLARE one 1");
        addCommand("FOR [ADD counter counter one; PRINT inc++] 5");
        addCommand("PRINT Done looping.");
        addCommand("PRINT Loop Counter: (counter)");
        addCommand("PRINT ---------------------------");
        //============================================================
        }
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