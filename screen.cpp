#include "screen.h"
#include "utils.h"

#include <iostream>
#include <vector>
#include <string>
using namespace std;


Screen::Screen(string name, string timeCreated) {
    this->name = name;
    this->curr_instruction = 0;
    this->num_instructions = 0;
    this->timeCreated = timeCreated;
    this->finished = false;
}

void Screen::displayDetails() {
    cout << "Process Name: " << name << "\n";
    cout << "Line of Instruction: " << curr_instruction << " / " << num_instructions << "\n";
    cout << "Time Created: " << timeCreated << "\n";
}

void Screen::startScreen() {
    string command;
    displayDetails();
    cout << "Enter a command: ";
    getline(cin, command);
    vector<string> words = split_sentence(command); // Entered command is a vector of strings

    while(words[0] != "exit"){ 
        if (words[0] == "clear") {
            clearScreen();
            displayDetails();
        } else if (words[0] == "exit") {
            clearScreen();
            return; // Exit the screen;
        } else if (words[0] == "print") { 
            /*Print command will print "Hello world from [screen name]" n times into a new text file that is named after the screen name
            format of the command is print n (with n as the number of prints)*/ 
            // For now, it sets the num_instructions to the value given by the user 
            if (words.size() != 2) {
                cout << "Invalid syntax. Use: print n" << "\n";
            } else {
                int n = stoi(words[1]); // stoi converts string to integer
                num_instructions = n;
            }
        } else {
            cout << "Invalid command." << "\n";
        }

        words.clear();
        cout << "Enter a command: ";
        getline(cin, command);
        words = split_sentence(command);
    }
}

string Screen::getName() const {
    return name;
}

string Screen::getTimeCreated() const {
    return timeCreated;
}

bool Screen::isFinished() const {
    return finished;
}