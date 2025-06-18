#include "Process.h"

#include <iostream>
#include <cctype>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <random>
#include <functional> 
#include <set>

Process::Process(const std::string& name, const std::string& p_id, const std::string& c_time)
    : processName(name),
      pid(p_id),
      creationTime(c_time),
      currentCommandIndex(0),
      totalInstructionLines(0),
      status(ProcessStatus::NEW),
      cpuCoreExecuting(-1),
      finishTime("N/A"),
      sleeping(false),
      wakeUpTime(0) {
}

int Process::EndFor(int forCommandIndex) const {
    if (commands[forCommandIndex].type != CommandType::FOR) {
        throw std::runtime_error("findMatchingEndFor called on non-FOR command.");
    }

    int nestingLevel = 0;
    for (int i = forCommandIndex + 1; i < commands.size(); ++i) {
        if (commands[i].type == CommandType::FOR) {
            nestingLevel++;
        } else if (commands[i].type == CommandType::END_FOR) {
            if (nestingLevel == 0) {
                return i;
            }
            nestingLevel--;
        }
    }
    return -1;
}

void Process::addCommand(const std::string& rawCommand) {
    static int forDepth = 0;

    std::stringstream ss(rawCommand);
    std::string commandTypeStr;
    ss >> commandTypeStr;

    CommandType type = CommandType::UNKNOWN;
    std::vector<std::string> args;

    std::transform(commandTypeStr.begin(), commandTypeStr.end(), commandTypeStr.begin(), ::toupper);

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
        type = CommandType::FOR;
        std::string loopVar;
        uint16_t startVal, endVal, stepVal;
        ss >> loopVar >> startVal >> endVal >> stepVal;
        args.push_back(loopVar);
        args.push_back(std::to_string(startVal));
        args.push_back(std::to_string(endVal));
        args.push_back(std::to_string(stepVal));
    }
    else if (commandTypeStr == "END_FOR") {
        type = CommandType::END_FOR;
    }

    commands.emplace_back(type, args, commands.size());
    totalInstructionLines = commands.size();
}

void Process::generateRandomCommands(int count) {
    std::vector<CommandType> possibleCommands = {
        CommandType::PRINT,
        CommandType::DECLARE,
        CommandType::ADD,
        CommandType::SUBTRACT,
        CommandType::SLEEP,
        CommandType::FOR
    };

    static int varCounter = 0;

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<int> distrib_cmd_type(0, possibleCommands.size() - 1);
    std::uniform_int_distribution<uint16_t> distrib_val(0, 255);
    std::uniform_int_distribution<int> distrib_sleep_duration(0, 255);
    std::uniform_int_distribution<int> distrib_fixed_var_index(0, 9);

    std::uniform_int_distribution<int> distrib_for_start(0, 4);
    std::uniform_int_distribution<int> distrib_for_end_offset(1, 5);
    std::uniform_int_distribution<int> distrib_for_step(1, 2);
    std::uniform_int_distribution<int> distrib_for_body_size(2, 5);

    std::uniform_int_distribution<int> distrib_loop_print_val(0, 99);
    std::uniform_int_distribution<int> distrib_loop_sleep_duration(0, 255);

    std::uniform_int_distribution<int> distrib_nesting_probability(0, 3);

    std::set<std::string> declaredVariableNames;

    std::function<void(int, int)> generateLoopBody =
        [&](int currentNestingDepth, int commandsForThisBody) {
        
        std::vector<CommandType> loopBodyPossibleCommands = {
            CommandType::PRINT,
            CommandType::ADD,
            CommandType::SUBTRACT,
            CommandType::SLEEP
        };

        if (currentNestingDepth < 3) {
            loopBodyPossibleCommands.push_back(CommandType::FOR);
        }

        std::uniform_int_distribution<int> distrib_loop_body_type(0, loopBodyPossibleCommands.size() - 1);

        for (int j = 0; j < commandsForThisBody; ++j) {
            if (commands.size() >= count) return;

            CommandType loopBodySelectedType = loopBodyPossibleCommands[distrib_loop_body_type(gen)];
            std::ostringstream body_cmd_oss;

            if (loopBodySelectedType == CommandType::FOR) {
                if (currentNestingDepth < 3 && distrib_nesting_probability(gen) > currentNestingDepth) {
                    int min_for_estimate = 1 + 1 + distrib_for_body_size.min();
                    if ((commands.size() + min_for_estimate) >= count) {
                        j--;
                        continue;
                    }

                    std::string nestedLoopVar = "loopNest" + std::to_string(currentNestingDepth) + "_" + std::to_string(distrib_fixed_var_index(gen));
                    uint16_t nestedStart = distrib_for_start(gen);
                    uint16_t nestedEnd = nestedStart + distrib_for_end_offset(gen);
                    uint16_t nestedStep = distrib_for_step(gen);
                    body_cmd_oss << "FOR " << nestedLoopVar << " " << nestedStart << " " << nestedEnd << " " << nestedStep;
                    addCommand(body_cmd_oss.str());

                    int nestedBodySize = distrib_for_body_size(gen);
                    generateLoopBody(currentNestingDepth + 1, nestedBodySize);

                    if (commands.size() < count) {
                         addCommand("END_FOR");
                    }
                    continue;
                }
            }
            
            switch (loopBodySelectedType) {
                case CommandType::PRINT:
                    body_cmd_oss << "PRINT \"Loop (depth " << currentNestingDepth << "): " << " iter " << j << " val " << distrib_loop_print_val(gen) << "\"";
                    break;
                case CommandType::ADD:
                case CommandType::SUBTRACT: {
                    std::string op = (loopBodySelectedType == CommandType::ADD) ? "ADD" : "SUBTRACT";
                    std::string var1, var2, destVar;

                    if (declaredVariableNames.empty()) {
                        if (commands.size() + 3 >= count) {
                            j--;
                            continue;
                        }
                        var1 = "var" + std::to_string(varCounter++);
                        addCommand("DECLARE " + var1 + " " + std::to_string(distrib_val(gen)));
                        declaredVariableNames.insert(var1);
                        if (commands.size() >= count) break;

                        var2 = "var" + std::to_string(varCounter++);
                        addCommand("DECLARE " + var2 + " " + std::to_string(distrib_val(gen)));
                        declaredVariableNames.insert(var2);
                        if (commands.size() >= count) break;
                    } else {
                        std::vector<std::string> currentVars(declaredVariableNames.begin(), declaredVariableNames.end());
                        std::uniform_int_distribution<size_t> var_pick_dist(0, currentVars.size() - 1);
                        var1 = currentVars[var_pick_dist(gen)];
                        var2 = currentVars[var_pick_dist(gen)];
                    }
                    
                    destVar = "res" + std::to_string(distrib_fixed_var_index(gen));
                    if (declaredVariableNames.find(destVar) == declaredVariableNames.end()) {
                         if (commands.size() + 1 >= count) {
                             j--;
                             continue;
                         }
                         addCommand("DECLARE " + destVar + " 0");
                         declaredVariableNames.insert(destVar);
                         if (commands.size() >= count) break;
                    }

                    body_cmd_oss << op << " " << var1 << " " << var2 << " " << destVar;
                    break;
                }
                case CommandType::SLEEP:
                    body_cmd_oss << "SLEEP " << distrib_loop_sleep_duration(gen);
                    break;
                default:
                    body_cmd_oss << "PRINT \"(Error: unexpected loop body command)\"";
                    break;
            }
            addCommand(body_cmd_oss.str());
        }
    };

    varCounter = 0; 
    commands.clear();
    variables.clear();
    declaredVariableNames.clear();

    int initialDeclaresCount = std::min(count / 5, 10);
    for (int i = 0; i < initialDeclaresCount; ++i) {
        if (commands.size() >= count) break;
        std::string varName = "var" + std::to_string(varCounter++);
        addCommand("DECLARE " + varName + " " + std::to_string(distrib_val(gen)));
        declaredVariableNames.insert(varName);
    }
    for (int i = 0; i <= distrib_fixed_var_index.max(); ++i) {
         if (commands.size() >= count) break;
         std::string resVarName = "res" + std::to_string(i);
         if (declaredVariableNames.find(resVarName) == declaredVariableNames.end()) {
            addCommand("DECLARE " + resVarName + " 0");
            declaredVariableNames.insert(resVarName);
         }
    }

    while (commands.size() < count) {
        CommandType selectedType = possibleCommands[distrib_cmd_type(gen)];
        std::ostringstream cmd_oss;

        if (selectedType == CommandType::DECLARE) {
            continue; 
        }

        if (selectedType == CommandType::FOR) {
            int min_for_for_estimate = 1 + 1 + distrib_for_body_size.min();
            if ((commands.size() + min_for_for_estimate * (distrib_nesting_probability.max() + 1)) > count) {
                continue;
            }
        }
        
        switch (selectedType) {
            case CommandType::PRINT: {
                int messageType = rand() % 3;
                if (messageType == 0) cmd_oss << "PRINT \"Msg: " << processName << " cmd " << commands.size() << "\"";
                else if (messageType == 1) cmd_oss << "PRINT \"Value is " << distrib_val(gen) << "\"";
                else cmd_oss << "PRINT \"Current overall command: " << commands.size() << "\"";
                addCommand(cmd_oss.str());
                break;
            }
            case CommandType::ADD:
            case CommandType::SUBTRACT: {
                std::string op = (selectedType == CommandType::ADD) ? "ADD" : "SUBTRACT";
                std::string var1, var2, destVar;

                if (declaredVariableNames.empty()) {
                    if (commands.size() + 3 >= count) {
                        continue;
                    }
                    var1 = "var" + std::to_string(varCounter++);
                    addCommand("DECLARE " + var1 + " " + std::to_string(distrib_val(gen)));
                    declaredVariableNames.insert(var1);
                    if (commands.size() >= count) break;

                    var2 = "var" + std::to_string(varCounter++);
                    addCommand("DECLARE " + var2 + " " + std::to_string(distrib_val(gen)));
                    declaredVariableNames.insert(var2);
                    if (commands.size() >= count) break;
                } else {
                    std::vector<std::string> currentVars(declaredVariableNames.begin(), declaredVariableNames.end());
                    std::uniform_int_distribution<size_t> var_pick_dist(0, currentVars.size() - 1);
                    var1 = currentVars[var_pick_dist(gen)];
                    var2 = currentVars[var_pick_dist(gen)];
                }
                
                destVar = "res" + std::to_string(distrib_fixed_var_index(gen));
                if (declaredVariableNames.find(destVar) == declaredVariableNames.end()) {
                    if (commands.size() + 1 >= count) {
                        continue;
                    }
                    addCommand("DECLARE " + destVar + " 0");
                    declaredVariableNames.insert(destVar);
                    if (commands.size() >= count) break;
                }

                cmd_oss << op << " " << var1 << " " << var2 << " " << destVar;
                addCommand(cmd_oss.str());
                break;
            }
            case CommandType::SLEEP: {
                int duration = distrib_sleep_duration(gen);
                cmd_oss << "SLEEP " << duration;
                addCommand(cmd_oss.str());
                break;
            }
            case CommandType::FOR: {
                std::string loopVar = "loopIdx" + std::to_string(distrib_fixed_var_index(gen));
                uint16_t start = distrib_for_start(gen);
                uint16_t end = start + distrib_for_end_offset(gen);
                uint16_t step = distrib_for_step(gen);
                cmd_oss << "FOR " << loopVar << " " << start << " " << end << " " << step;
                addCommand(cmd_oss.str());

                int bodySize = distrib_for_body_size(gen);
                generateLoopBody(1, bodySize);

                if (commands.size() < count) {
                    addCommand("END_FOR");
                }
                break;
            }
            case CommandType::UNKNOWN:
            case CommandType::END_FOR:
            default: {
                cmd_oss << "PRINT \"(Unknown or invalid command generated.)\"";
                addCommand(cmd_oss.str());
                break;
            }
        }
    }
    totalInstructionLines = commands.size();
}

std::string Process::resolvePrintMessage(const std::string& message) const {
    std::string resolved = message;
    size_t startPos = 0;
    while ((startPos = resolved.find('(', startPos)) != std::string::npos) {
        size_t endPos = resolved.find(')', startPos);
        if (endPos != std::string::npos) {
            std::string varName = resolved.substr(startPos + 1, endPos - startPos - 1);
            uint16_t value;
            if (getVariableValue(varName, value)) {
                resolved.replace(startPos, endPos - startPos + 1, std::to_string(value));
                startPos += std::to_string(value).length();
            } else {
                resolved.replace(startPos, endPos - startPos + 1, "(UNDEFINED_VAR)");
                startPos += std::string("(UNDEFINED_VAR)").length();
            }
        } else {
            break;
        }
    }
    return resolved;
}

const ParsedCommand* Process::getNextCommand() {
    if (currentCommandIndex >= commands.size() && loopStack.empty()) {
        return nullptr;
    }

    if (!loopStack.empty()) {
        LoopContext& currentLoop = loopStack.top();

        if (currentCommandIndex == currentLoop.endCommandIndex) {
            currentLoop.currentLoopValue += currentLoop.stepValue;
            setVariableValue(currentLoop.loopVarName, currentLoop.currentLoopValue);

            bool conditionMet;
            if (currentLoop.stepValue > 0) {
                conditionMet = (currentLoop.currentLoopValue <= currentLoop.endValue);
            } else if (currentLoop.stepValue < 0) {
                conditionMet = (currentLoop.currentLoopValue >= currentLoop.endValue);
            } else {
                conditionMet = false;
            }

            if (conditionMet) {
                currentCommandIndex = currentLoop.startCommandIndex;
                return &commands[currentCommandIndex++];
            } else {
                loopStack.pop();
                currentCommandIndex++; 
                return getNextCommand();
            }
        }
    }

    if (currentCommandIndex < commands.size()) {
        const ParsedCommand* currentParsedCommand = &commands[currentCommandIndex];

        switch (currentParsedCommand->type) {
            case CommandType::FOR: {
                const std::string& loopVarName = currentParsedCommand->args[0];
                uint16_t startVal = static_cast<uint16_t>(std::stoul(currentParsedCommand->args[1]));
                uint16_t endVal = static_cast<uint16_t>(std::stoul(currentParsedCommand->args[2]));
                uint16_t stepVal = static_cast<uint16_t>(std::stoul(currentParsedCommand->args[3]));

                int endForIndex = EndFor(currentCommandIndex);
                if (endForIndex == -1) {
                    throw std::runtime_error("FOR loop at index " + std::to_string(currentCommandIndex) + " has no matching END_FOR.");
                }

                declareVariable(loopVarName, startVal);

                bool initialConditionMet;
                if (stepVal > 0) {
                    initialConditionMet = (startVal <= endVal);
                } else if (stepVal < 0) {
                    initialConditionMet = (startVal >= endVal);
                } else {
                    initialConditionMet = (startVal == endVal);
                }

                if (initialConditionMet) {
                    loopStack.emplace(currentCommandIndex + 1,
                                      endForIndex,
                                      loopVarName,
                                      startVal,
                                      endVal,
                                      stepVal);
                } else {
                    currentCommandIndex = endForIndex; 
                }
                break;
            }
            case CommandType::END_FOR: {
                addLogEntry("WARNING: Encountered END_FOR command without active loop context. Advancing.");
                break;
            }
            case CommandType::DECLARE:
            case CommandType::ADD:
            case CommandType::SUBTRACT:
            case CommandType::SLEEP:
            case CommandType::PRINT:
            case CommandType::UNKNOWN:
            default: {
                break; 
            }
        }
        
        currentCommandIndex++;
        return currentParsedCommand;
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
    variables[varName] = value;
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

bool Process::isSleeping() const {
    return sleeping;
}

void Process::setSleeping(bool value) {
    sleeping = value;
}

void Process::setWakeUpTime(long long time) {
    wakeUpTime = time;
}

long long Process::getWakeUpTime() const {
    return wakeUpTime;
}