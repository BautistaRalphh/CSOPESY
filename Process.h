#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <sstream>

enum class ProcessStatus {
    NEW,
    IDLE,
    RUNNING,
    FINISHED,
    PAUSED
};

enum class CommandType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR_LOOP,
    UNKNOWN
};

// Structure to hold parsed command data
struct ParsedCommand {
    CommandType type;
    std::vector<std::string> args;
    // Future: additional fields for complex commands (e.g., nested instructions for FOR_LOOP)

    ParsedCommand(CommandType t = CommandType::UNKNOWN, std::vector<std::string> a = {})
        : type(t), args(a) {}
};

class Process {
private:
    std::string pid;
    std::string processName;
    std::vector<ParsedCommand> commands;
    int currentCommandIndex;
    int totalInstructionLines;
    std::string creationTime;
    ProcessStatus status;
    int cpuCoreExecuting;
    std::string finishTime;

    std::map<std::string, uint16_t> variables;

public:
    // Constructor
    Process(const std::string& name = "", const std::string& p_id = "", const std::string& c_time = "");

    // Methods to manage commands
    void addCommand(const std::string& rawCommand);
    void generateDummyPrintCommands(int count, const std::string& baseMessage);

    // Getters for process information
    const std::string& getPid() const { return pid; }
    const std::string& getProcessName() const { return processName; }
    const std::vector<ParsedCommand>& getCommands() const { return commands; }
    int getCurrentCommandIndex() const { return currentCommandIndex; }
    int getTotalInstructionLines() const { return totalInstructionLines; }
    std::string getCreationTime() const;
    ProcessStatus getStatus() const { return status; }
    int getCpuCoreExecuting() const; 
    const std::string& getFinishTime() const { return finishTime; }

    const ParsedCommand* getNextCommand() const;
    const ParsedCommand* getCommandAtIndex(int index) const;

    void setPid(const std::string& p_id) { pid = p_id; }
    void setProcessName(const std::string& name) { processName = name; }
    void setCurrentCommandIndex(int index) { currentCommandIndex = index; }
    void setTotalInstructionLines(int count) { totalInstructionLines = count; }
    void setCreationTime(const std::string& time) { creationTime = time; }
    void setStatus(ProcessStatus newStatus) { status = newStatus; }
    void setCpuCoreExecuting(int core); 
    void setFinishTime(const std::string& time) { finishTime = time; }

    void declareVariable(const std::string& varName, uint16_t value = 0);
    bool getVariableValue(const std::string& varName, uint16_t& value) const;
    void setVariableValue(const std::string& varName, uint16_t value);
    bool doesVariableExist(const std::string& varName) const;
};