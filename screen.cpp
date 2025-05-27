#include "screen.h"

#include <iostream>
#include <sstream>
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