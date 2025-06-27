#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <deque>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>
#include "screen.h"
#include "utils.h" 

using namespace std;

atomic<bool> schedulerRunning(false);  // Shared flag to signal scheduler to stop
thread schedulerThread;

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

void fcfsCore(vector<Screen>& screens, int coreNumber) {
    while (schedulerRunning) {
        for(auto& screen : screens) {
            if (!screen.isRunning() && !screen.isFinished()) {
                while (!screen.isFinished()) { // fcfs logic, process until finished
                    screen.doProcess(coreNumber);  
                }
                break;
            }
        }
    }
}

void rrCore(deque<Screen*>& queue, mutex& queueMutex, int coreNumber, int quantumCycles) {
    while (schedulerRunning) {
        Screen* screen = nullptr;

        {
            lock_guard<mutex> lock(queueMutex);
            if (!queue.empty()) {
                screen = queue.front();
                queue.pop_front();
            }
        }

        if (screen != nullptr && !screen->isFinished()) {
            // Simulate time slice: allow only `quantumCycles` number of instructions
            for (int i = 0; i < quantumCycles && !screen->isFinished(); ++i) {
                screen->doProcess(coreNumber);
            }

            // Move screen to the back of the deque if not finished
            if (!screen->isFinished()) {
                lock_guard<mutex> lock(queueMutex);
                screen->setRunningToFalse(); 
                queue.push_back(screen);
            }
        }

        this_thread::sleep_for(chrono::milliseconds(10)); 
    }
}

void schedulerStart(vector<Screen>& screens, int num_cpu, string scheduler) {
    cout << "scheduler-start command recognized." << "\n";
    schedulerRunning = true;
    
    if(scheduler == "fcfs") {
        vector<thread> cores;

        for (int i = 0; i < num_cpu; ++i) {
            cores.emplace_back(fcfsCore, ref(screens), i);
        }

        for (auto& core : cores) {
            if (core.joinable()) {
                core.join();
            }
        }
    } else if(scheduler == "rr") {
        deque<Screen*> screenQueue;
        mutex queueMutex;

        // Put screen pointers from vector into the deque
        for (auto& screen : screens) {
            if (!screen.isFinished()) {
                screenQueue.push_back(&screen);
            }
        }

        vector<thread> cores;
        int quantumCycles = 5; 

        for (int i = 0; i < num_cpu; ++i) {
            cores.emplace_back(rrCore, ref(screenQueue), ref(queueMutex), i, quantumCycles);
        }

        for (auto& core : cores) {
            if (core.joinable()) core.join();
        }
    }

    cout << "Scheduler finished.\n";
}

void schedulerStop() {
    cout << "scheduler-stop command recognized." << "\n";
    schedulerRunning = false;

    if (schedulerThread.joinable()) {
        schedulerThread.join();  // Wait for thread to finish
    }
}

void reportUtil() {
    cout << "report-util command recognized. Doing Something." << "\n";
}

int main() {
    string command;
    thread schedulerThread;
    printHeader();
    cout << "Enter a command: ";
    getline(cin, command);
    vector<string> words = split_sentence(command); // Entered command is a vector of strings
    vector<Screen> screens; 

    // Processes used for testing
    Screen p01("p01", getCurrentTime());
    Screen p02("p02", getCurrentTime());
    Screen p03("p03", getCurrentTime());
    Screen p04("p04", getCurrentTime());
    Screen p05("p05", getCurrentTime());
    Screen p06("p06", getCurrentTime());
    Screen p07("p07", getCurrentTime());
    Screen p08("p08", getCurrentTime());
    Screen p09("p09", getCurrentTime());
    Screen p10("p10", getCurrentTime());
    screens.push_back(p01);
    screens.push_back(p02);
    screens.push_back(p03);
    screens.push_back(p04);
    screens.push_back(p05);
    screens.push_back(p06);
    screens.push_back(p07);
    screens.push_back(p08);
    screens.push_back(p09);
    screens.push_back(p10);

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
                    cout << "---------------------------------------------------------------------------" << "\n";
                    cout << "Running processes:\n"; 
                    for (const auto& s : screens) {
                        if(s.isRunning()) {
                            cout << s.getName() << "    (" << s.getTimeCreated() << ")    " << "Core: " << s.getAssignedCore() << "    " << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
                        }
                    }
                    cout << "\nFinished processes:\n"; 
                    for (const auto& s : screens) {
                        if(s.isFinished()) {
                            cout << s.getName() << "    (" << s.getTimeCreated() << ")    " << "Finished    " << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
                        }
                    }
                    cout << "---------------------------------------------------------------------------" << "\n";
                } else {
                    cout << "Invalid option for screen command." << "\n";
                }
            }
        } else if (words[0] == "scheduler-start") { 
            if (!schedulerRunning) {
                schedulerThread = thread(schedulerStart, ref(screens), 4, "rr");
            } else {
                cout << "Scheduler is already running.\n";
            }
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

    if (schedulerRunning) {
        schedulerRunning = false;
        if (schedulerThread.joinable()) {
            schedulerThread.join();
        }
    }   

    return 0;
}