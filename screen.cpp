#include "screen.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
using namespace std;


Screen::Screen(string name, string timeCreated) {
    this->name = name;
    this->curr_instruction = 0;
    this->num_instructions = 100; // Default number of instructions, can be changed later
    this->timeCreated = timeCreated;
    this->running = false;
    this->finished = false;
}

void Screen::displayDetails() {
    cout << "Process Name: " << name << "\n";
    cout << "Line of Instruction: " << curr_instruction << " / " << num_instructions << "\n";
    cout << "Time Created: " << timeCreated << "\n";
}

void Screen::doProcess(int coreID) {
    // This function increments the curr_instruction and checks if the process is finished
    // It also prints the current time using getCurrentTime(), the core (thread) it is running on, and "Hello world from [screen name]". 
    // This gets printed to a text file named after the screen name.
    if (!finished) {
        if (!running){
            running = true; // Set running to true when the process starts
        }
        assignedCore = coreID; // Assign the core ID to the screen

        // Code to print to txt file (Not needed anymore I think)
        // ofstream outfile(name + ".txt", ios::app);
        // if (outfile.is_open()) {
        //     string currentTime = getCurrentTime();  // From utils.h

        //     outfile << "(" << currentTime << ") ";
        //     outfile << "Core: " << assignedCore << " ";
        //     outfile << "\"Hello world from " << name << "!\"\n";

        //     outfile.close();
        // } else {
        //     cerr << "Error: Could not open file " << name << ".txt for writing.\n";
        // }

        curr_instruction++;

        if (curr_instruction >= num_instructions) {
            finished = true; 
            running = false; 
        }

        this_thread::sleep_for(chrono::milliseconds(50)); // Delay to simulate processing time
    }
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

bool Screen::isRunning() const {
    return running;
}

int Screen::getCurrInstruction() const {
    return curr_instruction;
}

int Screen::getNumInstructions() const {
    return num_instructions;
}

int Screen::getAssignedCore() const {
    return assignedCore;
}

void Screen::setRunningToFalse() {
    running = false;
}