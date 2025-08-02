#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <stack>
#include <cstdlib>
#include <ctime>
#include <random> 
#include <chrono> 
#include <set> 

enum class ProcessStatus {
    NEW,
    READY,
    RUNNING,
    TERMINATED,
    PAUSED 
};

enum class CommandType {
    UNKNOWN,
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR,
    END_FOR,
    WRITE,
    READ
};

struct ParsedCommand {
    CommandType type;
    std::vector<std::string> args;
    int originalLineIndex;

    ParsedCommand(CommandType t = CommandType::UNKNOWN, std::vector<std::string> a = {}, int originalIdx = -1)
        : type(t), args(a), originalLineIndex(originalIdx) {}
};

struct LoopContext {
    int startCommandIndex;
    int endCommandIndex;
    std::string loopVarName;
    uint16_t startValue;
    uint16_t currentLoopValue;
    uint16_t endValue;
    uint16_t stepValue;

    LoopContext(int startIdx, int endIdx, const std::string& varName, uint16_t currentVal, uint16_t endVal, uint16_t stepVal)
        : startCommandIndex(startIdx), endCommandIndex(endIdx), loopVarName(varName),
          currentLoopValue(currentVal), endValue(endVal), stepValue(stepVal) {}
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
    uint32_t memoryRequired = 0;
    uint32_t pagesAllocated = 0;
    std::vector<std::string> splitInstructions(const std::string& block);
    std::string trim(const std::string& str);

    std::map<std::string, uint16_t> variables;
    std::map<uint32_t, uint16_t> memory; 
    std::vector<std::string> executionLog;

    std::stack<LoopContext> loopStack;

    bool sleeping;
    long long wakeUpTime;

    int EndFor(int forCommandIndex) const;

public:
    Process(const std::string& name = "", const std::string& p_id = "", const std::string& c_time = "");

    void addCommand(const std::string& rawCommand);
    void generateDummyPrintCommands(int count, const std::string& baseMessage);
    void generateRandomCommands(int count); 

    const std::string& getPid() const { return pid; }
    const std::string& getProcessName() const { return processName; }
    const std::vector<ParsedCommand>& getCommands() const { return commands; }
    int getCurrentCommandIndex() const { return currentCommandIndex; }
    int getTotalInstructionLines() const { return totalInstructionLines; }
    std::string getCreationTime() const;
    ProcessStatus getStatus() const { return status; }
    int getCpuCoreExecuting() const; 
    const std::string& getFinishTime() const { return finishTime; }
    const ParsedCommand* getNextCommand();

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

    // Memory operations
    void writeMemory(uint32_t address, uint16_t value);
    uint16_t readMemory(uint32_t address);

    void addLogEntry(const std::string& log);
    const std::vector<std::string>& getLogEntries() const;

    bool doesVariableExist(const std::string& varName) const;

    bool isSleeping() const;
    void setSleeping(bool value);
    void setWakeUpTime(long long time);
    long long getWakeUpTime() const;
    bool isLoopStackEmpty() const { return loopStack.empty(); } 

    void setMemory(uint32_t mem, uint32_t pages);
    uint32_t getMemoryRequired() const;
    uint32_t getPagesAllocated() const;
};