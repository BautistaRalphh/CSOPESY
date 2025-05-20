#include <iostream>
#include <string>
#include <limits>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>
#include <regex>

using namespace std;

struct Screen {
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

string getCurrentTimestamp() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", &tstruct);
    return buf;
}

void displayScreen(const Screen& screen) {
    cout << "Process Name: " << screen.processName << endl;
    cout << "Instruction: " << screen.currentInstructionLine << " / " << screen.totalInstructionLines << endl;
    cout << "Creation Time: " << screen.creationTime << endl;
}

string parseScreenName(const string& command) {
    regex screen_regex(R"(^screen\s+(-r|-s)\s+(\w+)$)");
    smatch match;
    if (regex_match(command, match, screen_regex) && match.size() == 3) {
        return match[2].str();
    }
    return "";
}

void createScreen(map<string, Screen>& screens, const string& command) {
    string name = parseScreenName(command);
    if (!name.empty() && regex_match(command, regex(R"(^screen\s+-s\s+\w+$)"))) {
        Screen newScreen;
        newScreen.processName = name;
        newScreen.currentInstructionLine = 10;  // Placeholder
        newScreen.totalInstructionLines = 100; // Placeholder
        newScreen.creationTime = getCurrentTimestamp();
        screens[name] = newScreen;
        cout << "Screen '" << name << "' created." << endl;
    } else {
        cout << "Invalid screen create command. Use 'screen -s <name>'" << endl;
    }
}

int main() {
    //Variables
    map<string, Screen> screens;
    string command;

    displayHeader();

    while (true) {
        cout << "Enter command: "; 
        getline(cin, command);

        regex screen_regex(R"(^screen\s+(-r|-s)\s+(\w+)$)");
        smatch match;

         if (command == "initialize") {
            cout << "'initialize' command recognized. Doing something." << endl;
        } else if (regex_match(command, match, screen_regex)) {
            string option = match[1].str();
            string screenName = match[2].str();

            if (option == "-r") {
                if (screens.count(screenName)) {
                    displayScreen(screens[screenName]);
                    while (true) {
                        cout << "Screen:\\> ";
                        getline(cin, command);
                        if (command == "exit") {
                            break;
                        } else {
                            cout << "Unknown screen command: " << command << endl;
                        }
                    }
                } else {
                    cout << "Screen '" << screenName << "' not found." << endl;
                }
            } else if (option == "-s") {
                createScreen(screens, command);
            } else {
                cout << "Invalid screen option." << endl;
            }
        } else if (command == "scheduler-test") {
            cout << "'scheduler-test' command recognized. Doing something." << endl;
        } else if (command == "scheduler-stop") {
            cout << "'scheduler-stop' command recognized. Doing something." << endl;
        } else if (command == "report-util") {
            cout << "'report-util' command recognized. Doing something." << endl;
        } else if (command == "clear") {
            cout << "\033[2J\033[1;1H";
            displayHeader();
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