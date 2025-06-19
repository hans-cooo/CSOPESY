#ifndef SCREEN_H
#define SCREEN_H

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

class Screen {
    string name;
    int curr_instruction;
    int num_instructions;
    string timeCreated;

    public:
        Screen(string name, string timeCreated);
        void displayDetails();
        void startScreen();
        string getName() const;
};

#endif