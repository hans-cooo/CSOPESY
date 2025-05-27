#include "screen.h"
#include "utils.h"

#include <iostream>
#include <vector>
#include <string>
using namespace std;


Screen::Screen(string name, string timeCreated) {
    this->name = name;
    this->instruction = "10/100"; // Placeholder for demonstration
    this->timeCreated = timeCreated;
}

void Screen::displayDetails() {
    cout << "Process Name: " << name << "\n";
    cout << "Line of Instruction: " << instruction << "\n";
    cout << "Time Created: " << timeCreated << "\n";
}

void Screen::startScreen() {
    string command;
    displayDetails();
    cout << "Enter a command: ";
    getline(cin, command);
    vector<string> words = split_sentence(command); // Entered command is a vector of strings

    while(words[0] != "exit"){ // Currently, the only command that exists for screen is exit
        cout << "Invalid command." << "\n";
    }

    words.clear();
        cout << "Enter a command: ";
        getline(cin, command);
        words = split_sentence(command);
}

