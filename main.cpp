#include <iostream>
#include <string>
#include <limits>

using namespace std;

int main()
{
    //Variables
    string command;

    //Header
    cout << "  _____  _____  ____  _____  ______  _______    __" << endl;
    cout << " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____\\ \\  / /" << endl;
    cout << "| |    | (___ | |  | | |__) | |__  | (___  \\ \\/ / " << endl;
    cout << "| |     \\___ \\| |  | | ___/ |  __|  \\___ \\  \\  /  " << endl;
    cout << "| |____ ____) | |__| | |    | |____ ____) |  | |   " << endl;
    cout << " \\_____|_____/ \\____/|_|    |______|_____/   |_|   " << endl;
    cout << "\033[32mHello, welcome to CSOPESY command line\033[0m" << endl;
    cout << "\033[33mType 'exit' to quit, 'clear' to clear the screen\033[0m" << endl;
    cout << endl;

    while (true) {
        cout << "Enter command: ";
        getline(cin, command);

        if (command == "initialize") {
            cout << "'initialize' command recognized. Doing something." << endl;
        } else if (command == "screen") {
            cout << "'screen' command recognized. Doing something." << endl;
        } else if (command == "scheduler-test") {
            cout << "'scheduler-test' command recognized. Doing something." << endl;
        } else if (command == "scheduler-stop") {
            cout << "'scheduler-stop' command recognized. Doing something." << endl;
        } else if (command == "report-util") {
            cout << "'report-util' command recognized. Doing something." << endl;
        } else if (command == "clear") {
            cout << "\033[2J\033[1;1H";
            cout << "  _____  _____  ____  _____  ______  _______    __" << endl;
            cout << " / ____|/ ____|/ __ \\|  __ \\|  ____|/ ____\\ \\  / /" << endl;
            cout << "| |    | (___ | |  | | |__) | |__  | (___  \\ \\/ / " << endl;
            cout << "| |     \\___ \\| |  | | ___/ |  __|  \\___ \\  \\  /  " << endl;
            cout << "| |____ ____) | |__| | |    | |____ ____) |  | |   " << endl;
            cout << " \\_____|_____/ \\____/|_|    |______|_____/   |_|   " << endl;
            cout << "\033[32mHello, welcome to CSOPESY command line\033[0m" << endl;
            cout << "\033[33mType 'exit' to quit, 'clear' to clear the screen\033[0m" << endl;
            cout << endl;
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