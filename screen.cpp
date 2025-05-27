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
