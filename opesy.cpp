#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <chrono>
#include "screen.h"
#include "utils.h"

using namespace std;

void printHeader() {
    cout << "   ____   ____    _____   ____    ____   ____   __   __" << "\n";
    cout << "  / ___/ / ___|  /  _  | |  _ \\  |  __/ / ___|  \\ \\ / /" << "\n";
    cout << " | |     | |__   | | | | | |_| | | |_   | |__    \\ V / " << "\n";
    cout << " | |     \\___ \\  | | | | |  __/  |  _|  \\___ \\    | |  " << "\n";
    cout << " | |____  ___| | | |_| | | |     | |__   ___| |   | |  " << "\n";
    cout << "  \\____| |____/  |____/  |_|     |____| |____/    |_|  " << "\n";

    cout << "Hello, Welcome to CSOPESY commandline!" << "\n";
    cout << "Type 'exit' to quit, and 'clear' to clear the screen" << "\n";
}

void initialize() {
    cout << "initialize command recognized. Doing Something." << "\n";
}

void schedulerStart(vector<Screen>& screens, int num_cpu, string scheduler) {
    cout << "scheduler-start command recognized. Doing Something." << "\n"; // Temporary, used to test doProcess function
    for (auto& screen : screens) {
        while(!screen.isFinished()) {
            screen.doProcess();
        }
    }
}

void schedulerStop() {
    cout << "scheduler-stop command recognized. Doing Something." << "\n";
}

void reportUtil() {
    cout << "report-util command recognized. Doing Something." << "\n";
}

int main() {
    string command;
    printHeader();
    cout << "Enter a command: ";
    getline(cin, command);
    vector<string> words = split_sentence(command); // Entered command is a vector of strings
    vector<Screen> screens; 

    while (words[0] != "exit") {
        if (words[0] == "clear") {
            clearScreen();
            printHeader();
        } else if (words[0] == "initialize") {
            initialize();
        } else if (words[0] == "screen") {
            if(words.size() < 2) { // Check if screen has less than 2 arguments
                cout << "Invalid syntax." << "\n";
            }
            else{
                if(words[1] == "-r") { // Behavior to go to created screen
                    bool exists = false;
                    for (auto& screen : screens) {
                        if (screen.getName() == words[2]) {
                            exists = true;
                            break;
                        }
                    }
                    if (exists) {
                        for (auto& screen : screens) {
                            if (screen.getName() == words[2]) {
                                clearScreen();
                                screen.startScreen();
                                break;
                            }
                        }
                        clearScreen();
                        printHeader();
                    } else {
                        cout << "Screen with name '" << words[2] << "' does not exist." << "\n";
                    }

                } else if (words[1] == "-s") { // Behavior to create a screen
                    bool exists = false;
                    for (const auto& screen : screens) {
                        if (screen.getName() == words[2]) {
                            cout << "Screen with name '" << words[2] << "' already exists." << "\n";
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        Screen newScreen(words[2], getCurrentTime());
                        screens.push_back(newScreen);
                        cout << "Screen '" << words[2] << "' created at " << getCurrentTime() << "." << "\n";

                        // Print all screens (might change this to a different command later)
                        cout << "Current screens:\n";
                        for (const auto& s : screens) {
                            cout << "- " << s.getName() << "\n";
                        }
                    }
                } else if (words[1] == "-ls"){ // Behavior to list all screens
                    cout << "Running processes:\n"; 
                    for (const auto& s : screens) {
                        if(s.isRunning()) {
                            cout << s.getName() << "    (" << s.getTimeCreated() << ")    " << "Core: 0    " << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
                        }
                    }
                    cout << "\nFinished processes:\n"; 
                    for (const auto& s : screens) {
                        if(s.isFinished()) {
                            cout << s.getName() << "    (" << s.getTimeCreated() << ")    " << "Finished    " << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
                        }
                    }
                } else {
                    cout << "Invalid option for screen command." << "\n";
                }
            }
        } else if (words[0] == "scheduler-start") { 
            // Needs to be a separate thread so commands like screen -ls can be used while the scheduler is running
            schedulerStart(screens, 4, "fcfs"); 
        } else if (words[0] == "scheduler-stop") {
            schedulerStop();
        } else if (words[0] == "report-util") {
            reportUtil();
        } else {
            cout << "Invalid command." << "\n";
        }
        words.clear();
        cout << "Enter a command: ";
        getline(cin, command);
        words = split_sentence(command);
    }
    return 0;
}