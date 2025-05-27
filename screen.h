#ifndef SCREEN_H
#define SCREEN_H

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

class Screen {
    string name;
    string instruction;
    string timeCreated;

    public:
        Screen(string name, string timeCreated);
        void displayDetails();

};

#endif