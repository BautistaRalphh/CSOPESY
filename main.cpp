#include <iostream>
#include <string>
#include <limits>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>
#include <regex>

using namespace std;

struct Process {
    string processName;
    int currentInstructionLine;
    int totalInstructionLines;
    string creationTime;
};

void displayHeader() {
    cout << "  _____  _____  ____  _____  ______  _______    __" << endl;
    cout << " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____\\ \\  / /" << endl;
    cout << "| |    | (___ | |  | | |__) | |__  | (___  \\ \\/ / " << endl;
    cout << "| |     \\___ \\| |  | | ___/ |  __|  \\___ \\  \\  /  " << endl;
    cout << "| |____ ____) | |__| | |    | |____ ____) |  | |   " << endl;
    cout << " \\_____|_____/ \\____/|_|    |______|_____/   |_|   " << endl;
    cout << "\033[32mHello, welcome to CSOPESY command line\033[0m" << endl;
    cout << "\033[33mType 'exit' to quit, 'clear' to clear the screen\033[0m" << endl;
    cout << endl;
}

void displayScreenCommands() {
    cout << "The following are the available process commands:" << endl;
    cout << "  process -ls       : List all processes" << endl;
    cout << "  process -s <name> : Create a new process" << endl;
    cout << "  process -r <name> : Display an existing process" << endl;
    cout << endl;
}

string getCurrentTimestamp() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", &tstruct);
    return buf;
}

void displayProcess(const Process& process) {
    cout << "Process Name: " << process.processName << endl;
    cout << "Instruction: " << process.currentInstructionLine << " / " << process.totalInstructionLines << endl;
    cout << "Creation Time: " << process.creationTime << endl;
    cout << "Type 'exit' to return to main menu" << endl;
}

string parseProcessName(const string& command) {
    regex process_regex(R"(^screen\s+(-r|-s)\s+(\w+)$)");
    smatch match;
    if (regex_match(command, match, process_regex) && match.size() == 3) {
        return match[2].str(); 
    }
    return "";
}

bool createProcess(map<string, Process>& processes, const string& command, string& createdProcessName) {
    string name = parseProcessName(command); 
    if (!name.empty() && regex_match(command, regex(R"(^screen\s+-s\s+\w+$)"))) {
        if (processes.count(name)) { 
            cout << "Error: Process '" << name << "' already exists." << endl;
            return false;
        }
        Process newProcess; 
        newProcess.processName = name;
        newProcess.currentInstructionLine = 10;  // Placeholder
        newProcess.totalInstructionLines = 100; // Placeholder
        newProcess.creationTime = getCurrentTimestamp();
        processes[name] = newProcess; 
        cout << "Process '" << name << "' created." << endl;
        createdProcessName = name; 
        return true;
    } else {
        cout << "Invalid process create command. Use 'screen -s <name>'" << endl;
        return false;
    }
}

int main() {
    //Variables
    map<string, Process> processes;
    string command;

    displayHeader();

    while (true) {
        cout << "Enter a command: ";
        getline(cin, command);

        regex process_command_regex(R"(^screen\s+(-r|-s)\s+(\w+)$)");
        smatch match;

        if (command == "initialize") {
            cout << "'initialize' command recognized. Doing something." << endl;
        }  else if (command == "screen") { 
            displayScreenCommands();
        }  else if (regex_match(command, match, process_command_regex)) {
            string option = match[1].str();    
            string processName = match[2].str();

            if (option == "-r") {
                if (processes.count(processName)) {
                    cout << "\033[2J\033[1;1H";
                    displayProcess(processes[processName]); 
                    string process_cmd;
                    while (true) {
                        cout << endl;
                        cout << "root:\\> "; 
                        getline(cin, process_cmd);
                        if (process_cmd == "exit") {
                            cout << "\033[2J\033[1;1H";
                            break;
                        } else {
                            cout << "Unknown process command: " << process_cmd << endl; 
                        }
                    }
                } else {
                    cout << "Process '" << processName << "' not found." << endl;
                }
            } else if (option == "-s") {
                string createdProcess;
                if (createProcess(processes, command, createdProcess)) {
                    cout << "\033[2J\033[1;1H";
                    displayProcess(processes[createdProcess]);
                    string process_cmd;
                    while (true) {
                        cout << endl;
                        cout << "root:\\> "; 
                        getline(cin, process_cmd);
                        if (process_cmd == "exit") {
                            cout << "\033[2J\033[1;1H";
                            break;
                        } else {
                            cout << "Unknown process command: " << process_cmd << endl; 
                        }
                    }
                }
            } else {
                cout << "Invalid screen option. Valid commands are the following: screen -ls, screen -s <name>, screen -r <name>." << endl;
            }
        } else if (command == "scheduler-test") {
            cout << "'scheduler-test' command recognized. Doing something." << endl;
        } else if (command == "scheduler-stop") {
            cout << "'scheduler-stop' command recognized. Doing something." << endl;
        } else if (command == "report-util") {
            cout << "'report-util' command recognized. Doing something." << endl;
        } else if (command == "clear") {
            cout << "\033[2J\033[1;1H";
        } else if (command == "exit") {
            cout << "Exiting CSOPESY." << endl;
            break;
        } else {
            cout << "Unknown command: " << command << endl;
        }
        cout << endl;
    }

    return 0;
}