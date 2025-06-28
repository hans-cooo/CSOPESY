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
#include <fstream>
#include <sstream>
#include <iomanip> 
#include <random>
#include "screen.h"
#include "utils.h" 
#include "config.h"
Config config;

using namespace std;

atomic<bool> schedulerRunning(false);  // Shared flag to signal scheduler to stop
thread schedulerThread;
bool isInitialized = false;
atomic<int> generatedProcessCount(0);
const int maxGeneratedProcesses = 50;

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

int generateInstructions(int min_ins, int max_ins) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min_ins, max_ins);

    return dist(gen);
}


void initialize(int& num_cpu, string& scheduler, int& quantumCycles,
                int& batchProcessFreq, int& min_ins, int& max_ins, int& delayPerExec) {
    cout << "initialize command recognized.\n";
    if (config.loadFromFile("config.txt")) {

        // Assign to local variables in main
        num_cpu         = config.num_cpu;
        scheduler       = config.scheduler;
        quantumCycles   = config.quantumCycles;
        batchProcessFreq = config.batchProcessFreq;
        min_ins         = config.min_ins;
        max_ins         = config.max_ins;
        delayPerExec    = config.delayPerExec;
    }
}


void fcfsCore(vector<Screen>& screens, int coreNumber, int batchProcessFreq, int min_ins, int max_ins) {
    int cycleCounter = 0;

    while (schedulerRunning) {
        for(auto& screen : screens) {
            if (!screen.isRunning() && !screen.isFinished()) {
                while (!screen.isFinished()) { // fcfs logic, process until finished
                    screen.doProcess(coreNumber);  
                    cycleCounter++;
                }
                break;
            }
        }
    }
}

// cpuCycle % delayPerExec == 0

void rrCore(deque<Screen*>& queue, mutex& queueMutex, vector<Screen>& screens, mutex& screensMutex, 
    int coreNumber, int quantumCycles,
    int batchProcessFreq, int min_ins, int max_ins) {
    int cycleCounter = 0;

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
            for (int i = 0; i < quantumCycles && !screen->isFinished(); ++i) {
                screen->doProcess(coreNumber);
                cycleCounter++;
            }

            if (!screen->isFinished()) {
                lock_guard<mutex> lock(queueMutex);
                screen->setRunningToFalse();
                queue.push_back(screen);
            }
        }

        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

void schedulerStart(vector<Screen>& screens, int num_cpu, string scheduler, int quantumCycles, int batchProcessFreq, int min_ins, int max_ins) {
    cout << "scheduler-start command recognized." << "\n";
    schedulerRunning = true;

    // Generate processes
    if (generatedProcessCount == 0) {
        for (int i = 1; i <= maxGeneratedProcesses; ++i) {
            string name = "p" + to_string(i);
            int ins = generateInstructions(min_ins, max_ins);
            Screen newScreen(name, getCurrentTime(), ins);
            screens.push_back(newScreen);
            ++generatedProcessCount;
            cout << "Generated: " << name << " with " << ins << " instructions.\n";
        }
    }
    
    if(scheduler == "fcfs") {
        vector<thread> cores;

        for (int i = 0; i < num_cpu; ++i) {
            cores.emplace_back(fcfsCore, ref(screens), i, batchProcessFreq, min_ins, max_ins);
        }

        for (auto& core : cores) {
            if (core.joinable()) {
                core.join();
            }
        }
    } else if(scheduler == "rr") {
        deque<Screen*> screenQueue;
        mutex queueMutex;
        mutex screensMutex;

        // Put screen pointers from vector into the deque
        for (auto& screen : screens) {
            if (!screen.isFinished()) {
                screenQueue.push_back(&screen);
            }
        }

        vector<thread> cores;

        for (int i = 0; i < num_cpu; ++i) {
            cores.emplace_back(rrCore, ref(screenQueue), ref(queueMutex), ref(screens), ref(screensMutex), i, quantumCycles, batchProcessFreq, min_ins, max_ins);
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

void reportUtil(const vector<Screen>& screens) {
    cout << "report-util command recognized." << "\n";
    reportUtilToFile(screens, "report.txt");
    cout << "Process utilization report saved to report.txt\n";
}

int main() {
    string command;
    thread schedulerThread;

    int num_cpu;
    string scheduler;
    int quantumCycles;
    int batchProcessFreq;
    int min_ins;
    int max_ins;
    int delayPerExec;

    int numInstructions;

    printHeader();
    cout << "Enter a command: ";
    getline(cin, command);
    vector<string> words = split_sentence(command); // Entered command is a vector of strings
    vector<Screen> screens; 

    // Debugging for initialize to work
    // initialize(num_cpu, scheduler, quantumCycles, batchProcessFreq, min_ins, max_ins, delayPerExec);

    // Maybe run these processes inside the initialize function as well for Debugging?

    // // Debugging, Processes used for testing
    // Screen p01("p01", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // Screen p02("p02", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // Screen p03("p03", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // Screen p04("p04", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // Screen p05("p05", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // Screen p06("p06", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // Screen p07("p07", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // Screen p08("p08", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // Screen p09("p09", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // Screen p10("p10", getCurrentTime(), generateInstructions(min_ins, max_ins));
    // screens.push_back(p01);
    // screens.push_back(p02);
    // screens.push_back(p03);
    // screens.push_back(p04);
    // screens.push_back(p05);
    // screens.push_back(p06);
    // screens.push_back(p07);
    // screens.push_back(p08);
    // screens.push_back(p09);
    // screens.push_back(p10);

    while (words[0] != "exit") {
        if (words[0] == "clear") {
            clearScreen();
            printHeader();
        } else if (words[0] == "initialize") {
            initialize(num_cpu, scheduler, quantumCycles, batchProcessFreq, min_ins, max_ins, delayPerExec);
            // Debuggging
            // cout << "OVER HERE LOOK AT ME GOOOOOOOOOOOOOOOOO play Yakuza 0" << "\n";
            cout << "num_cpu: " << num_cpu << "\n";
            cout << "scheduler: " << scheduler << "\n";
            cout << "quantumCycles: " << quantumCycles << "\n";
            cout << "batchProcessFreq: " << batchProcessFreq << "\n";
            cout << "min_ins: " << min_ins << "\n";
            cout << "max_ins: " << max_ins << "\n";
            cout << "delayPerExec: " << delayPerExec << "\n";
            cout << "test numInstructions: " << generateInstructions(min_ins, max_ins) << "\n";
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
                        Screen newScreen(words[2], getCurrentTime(), generateInstructions(min_ins, max_ins));
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
                schedulerThread = thread(schedulerStart, ref(screens), num_cpu, 
                scheduler, quantumCycles, batchProcessFreq, min_ins, max_ins);
            } else {
                cout << "Scheduler is already running.\n";
            }
        } else if (words[0] == "scheduler-stop") {
            schedulerStop();
        } else if (words[0] == "report-util") {
            reportUtil(screens);
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