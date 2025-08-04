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
#include <unordered_set>
#include <optional>
#include <algorithm>
#include "screen.h"
#include "utils.h" 
#include "config.h"
#include <filesystem>
Config config;

using namespace std;

atomic<bool> schedulerRunning(false);  // Shared flag to signal scheduler to stop
thread schedulerThread;
bool isInitialized = false;
atomic<int> generatedProcessCount(0);
const int maxGeneratedProcesses = 50;
mutex memoryMutex;
namespace fs = std::filesystem;

struct MemoryBlock {
    string name;                 // Owner screen name or "NULL"
    optional<uint16_t> data;     // Placeholder for later use (read, write, add)
};

vector<MemoryBlock> memory;  // Global memory vector
deque<string> pageLoadOrder;
int pagesPagedIn = 0;
int pagesPagedOut = 0;

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
                int& batchProcessFreq, int& min_ins, int& max_ins, int& delayPerExec, int& max_overall_mem, int& mem_per_frame, int& mem_per_proc, bool& isInitialized) {
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
        max_overall_mem = config.max_overall_mem;
        mem_per_frame   = config.mem_per_frame;
        mem_per_proc    = config.mem_per_proc;
        memory.resize(max_overall_mem, {"NULL", nullopt});  // Resize memory to fit the maximum overall memory
        isInitialized = true;
    }
}

void evictOldestScreen() {
    lock_guard<mutex> lock(memoryMutex); 

    if (!pageLoadOrder.empty()) {
        string toRemove = pageLoadOrder.front();
        pageLoadOrder.pop_front();

        for (auto& block : memory) {
            if (block.name == toRemove) {
                block.name = "NULL";
                block.data = nullopt;
            }
        }

        pagesPagedOut++;
    }
}

bool allocateMemory(Screen* screen, int requiredBlocks) {
    lock_guard<mutex> lock(memoryMutex); 

    int available = 0;
    for (const auto& block : memory) {
        if (block.name == "NULL") available++;
    }

    while (available < requiredBlocks && !pageLoadOrder.empty()) {
        evictOldestScreen();
        available = 0;
        for (const auto& block : memory) {
            if (block.name == "NULL") available++;
        }
    }

    if (available < requiredBlocks) return false;

    int allocated = 0;
    for (auto& block : memory) {
        if (block.name == "NULL") {
            block.name = screen->getName();
            block.data = nullopt;
            allocated++;
            if (allocated == requiredBlocks) break;
        }
    }

    pageLoadOrder.push_back(screen->getName());
    pagesPagedIn++;
    return true;
}


void deallocateMemory(const string& screenName) {
    lock_guard<mutex> lock(memoryMutex);

    for (auto& block : memory) {
        if (block.name == screenName) {
            block.name = "NULL";
            block.data = nullopt;
        }
    }

    auto it = find(pageLoadOrder.begin(), pageLoadOrder.end(), screenName);
    if (it != pageLoadOrder.end()) {
        pageLoadOrder.erase(it);
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

void rrCore(deque<Screen*>& queue, mutex& queueMutex, vector<Screen>& screens, mutex& screensMutex, 
    int coreNumber, int quantumCycles,
    int batchProcessFreq, int min_ins, int max_ins) {

    int cycleCounter = 0;
    fs::create_directory("txt");

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

            if (screen->getMemStartIndex() == -1) {
                if (!allocateMemory(screen, screen->getRequiredMemory())) {
                    // Could not allocate memory, requeue and skip
                    lock_guard<mutex> lock(queueMutex);
                    queue.push_back(screen);
                    continue;
                }
            }

            for (int i = 0; i < quantumCycles && !screen->isFinished(); ++i) {
                screen->doProcess(coreNumber);
                cycleCounter++;

                // 🔽 Create memory snapshot log per quantum cycle
                {
                    lock_guard<mutex> lock(screensMutex); // Safe access to shared data

                    // Count processes in memory
                    int memCount = 0;
                    for (const Screen& s : screens) {
                        if (s.getMemStartIndex() != -1 && !s.isFinished()) {
                            memCount++;
                        }
                    }

                    // Count unused memory blocks (external fragmentation)
                    int unusedFrames = 0;
                    for (const auto& block : memory) {
                        if (block.name == "NULL") {
                            unusedFrames++;
                        }
                    }

                    // File output
                    string filename = "txt/memory_stamp_core" + to_string(coreNumber) + "_cycle" + to_string(cycleCounter) + ".txt";
                    ofstream outfile(filename);
                    if (outfile.is_open()) {
                        outfile << "Timestamp: " << getCurrentTime() << "\n";
                        outfile << "Core ID: " << coreNumber << "\n";
                        outfile << "Quantum Cycle: " << cycleCounter << "\n";
                        outfile << "Processes in Memory: " << memCount << "\n";
                        outfile << "Total External Fragmentation: " << unusedFrames << " KB\n\n";
                        outfile.close();
                    }
                }
            }

            if (screen->isFinished()) {
                deallocateMemory(screen->getName());
            } else {
                lock_guard<mutex> lock(queueMutex);
                screen->setRunningToFalse();
                queue.push_back(screen);
            }
        }

        this_thread::sleep_for(chrono::milliseconds(10));
    }
}





void schedulerStart(vector<Screen>& screens, int num_cpu, string scheduler, int quantumCycles, int batchProcessFreq, int min_ins, int max_ins, int mem_per_proc) {
    cout << "scheduler-start command recognized." << "\n";
    schedulerRunning = true;

    // Generate processes
    if (generatedProcessCount == 0) {
        for (int i = 1; i <= maxGeneratedProcesses; ++i) {
            string name = "p" + to_string(i);
            int ins = generateInstructions(min_ins, max_ins);
            Screen newScreen(name, getCurrentTime(), ins, mem_per_proc);
            screens.push_back(newScreen);
            ++generatedProcessCount;
            // cout << "Generated: " << name << " with " << ins << " instructions.\n";
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
            cores.emplace_back(rrCore, ref(screenQueue), ref(queueMutex), ref(screens), 
            ref(screensMutex), i, quantumCycles, batchProcessFreq, min_ins, max_ins);
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

void reportUtil(const vector<Screen>& screens, int num_cpu) {
    cout << "report-util command recognized." << "\n";
    reportUtilToFile(screens, "report.txt", num_cpu);
    cout << "Process utilization report saved to report.txt\n";
}

int main() {
    string command;

    int num_cpu;
    string scheduler;
    int quantumCycles;
    int batchProcessFreq;
    int min_ins;
    int max_ins;
    int delayPerExec;
    int numInstructions;
    int max_overall_mem;
    int mem_per_frame;
    int mem_per_proc;

    printHeader();
    cout << "Enter a command: ";
    getline(cin, command);
    vector<string> words = split_sentence(command); // Entered command is a vector of strings
    vector<Screen> screens; 

    while (words[0] != "exit") {
        if (words[0] == "clear") {
            clearScreen();
            printHeader();
        } else if (words[0] == "initialize") {
            initialize(num_cpu, scheduler, quantumCycles, batchProcessFreq, min_ins, max_ins, delayPerExec, max_overall_mem, mem_per_frame, mem_per_proc, isInitialized);
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
                        Screen newScreen(words[2], getCurrentTime(), generateInstructions(min_ins, max_ins), mem_per_proc);
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
                    unordered_set<int> activeCores;
                    for (const auto& s : screens) {
                        if(s.isRunning()) {
                            cout << s.getName() << "    (" << s.getTimeCreated() << ")    " << "Core: " << s.getAssignedCore() << "    " << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
                            
                            int coreID = s.getAssignedCore();
                            if (coreID >= 0){
                                activeCores.insert(coreID);
                            }
                            
                        }
                    }
                    cout << "\nFinished processes:\n"; 
                    for (const auto& s : screens) {
                        if(s.isFinished()) {
                            cout << s.getName() << "    (" << s.getTimeCreated() << ")    " << "Finished    " << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
                        }
                    }
                    float utilization = (num_cpu > 0) ? (static_cast<float>(activeCores.size()) / num_cpu) * 100.0f : 0.0f;
                    cout << "\nCPU Utilization: " << activeCores.size() << " / " << num_cpu << " cores active (" << utilization << "%)\n";

                    cout << "---------------------------------------------------------------------------" << "\n";
                } else {
                    cout << "Invalid option for screen command." << "\n";
                }
            }
        } else if (words[0] == "scheduler-start") { 
            if (!schedulerRunning) {
                if (isInitialized) {
                    schedulerThread = thread(schedulerStart, ref(screens), num_cpu, 
                    scheduler, quantumCycles, batchProcessFreq, min_ins, max_ins, mem_per_proc);
                } else {
                    cout << "Not yet initialized. Run 'initialize' first.\n";
                }
            } else {
                cout << "Scheduler is already running.\n";
            }
        } else if (words[0] == "scheduler-stop") {
            schedulerStop();
        } else if (words[0] == "report-util") {
            reportUtil(screens, num_cpu);
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