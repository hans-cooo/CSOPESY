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
    int assignedCore = -1; // -1 means unassigned
    string timeCreated;
    bool finished;
    bool running;
    vector<string> logs;
    int requiredMemory; 
    int memStartIndex = -1; // Index when still unassigned
    string customInstructions; // Custom instructions for the screen

    public:
        Screen(string name, string timeCreated, int numInstructions, int requiredMemory);
        void displayDetails();
        void startScreen();
        void doProcess(int coreID);
        string getName() const;
        string getTimeCreated() const;
        bool isFinished() const;
        bool isRunning() const;
        int getCurrInstruction() const;
        int getNumInstructions() const;
        int getAssignedCore() const;
        void setRunningToFalse();
        int getRequiredMemory() const;
        int getMemStartIndex() const;
        void setMemStartIndex(int index) { memStartIndex = index; }
        void setCustomInstructions(const string& instructions);
};

#endif