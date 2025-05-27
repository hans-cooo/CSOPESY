#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <chrono>
#include "screen.h"

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

void schedulerTest() {
    cout << "scheduler-test command recognized. Doing Something." << "\n";
}

void schedulerStop() {
    cout << "scheduler-stop command recognized. Doing Something." << "\n";
}

void reportUtil() {
    cout << "report-util command recognized. Doing Something." << "\n";
}

vector<string> split_sentence(string sen) {
  
    // Vector to store the words
    vector<string> words;

    // Temporary string to hold each word
    string word = "";

    // Iterate through each character in the sentence
    for (char c : sen) {
        if (c == ' ') {
          
            // If a space is found, add the word to the vector
            words.push_back(word);
          
            // Reset the word
            word = "";
        }
        else {
            // Append the character to the current word
            word += c;
        }
    }

    // Add the last word to the vector
    if (!word.empty()) {
        words.push_back(word);
    }

    // Return the vector containing words
    return words;
}

string getCurrentTime() {
    auto now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now);
    tm *now_tm = localtime(&now_c);

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%m/%d/%Y, %I:%M:%S %p", now_tm);
    return string(buffer);
}

int main() {
    string command;
    printHeader();
    cout << "Enter a command: ";
    getline(cin, command);
    vector<string> words = split_sentence(command); // Entered command is a vector of strings

    while (words[0] != "exit") {
        if (words[0] == "clear") {
            cout << "\033[2J\033[1;1H"; // ANSI escape code to clear the screen
            printHeader();
        } else if (words[0] == "initialize") {
            initialize();
        } else if (words[0] == "screen") {
            if(words.size() != 3) { // Check if screen has 3 arguments
                cout << "Invalid syntax." << "\n";
            }
            else{
                if(words[1] == "-r") {
                    cout << "Screen command recognized with -r option. Doing Something." << "\n"; // Add behavior for -r option
                } else if (words[1] == "-s") {
                    cout << "Screen command recognized with -s option. Doing Something." << "\n"; // Add behavior for -s option
                } else {
                    cout << "Invalid option for screen command." << "\n";
                }
            }
        } else if (words[0] == "scheduler-test") {
            schedulerTest();
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