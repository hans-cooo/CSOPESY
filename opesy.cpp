#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <deque>
#include <queue>
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
#include <map>
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
recursive_mutex memoryMutex;
namespace fs = std::filesystem;

struct MemoryBlock {
    string name;                 // Owner screen name or "NULL"
    optional<uint16_t> data;     // Placeholder for later use (read, write, add)
};

vector<MemoryBlock> memory;  // Global memory vector
queue<Screen*> fifoMemoryQueue;

std::mutex coutMutex; // For logging

atomic<int> totalIdleTicks(0);
atomic<int> totalActiveTicks(0);
atomic<int> totalCpuTicks(0);
atomic<int> memoryPagesPagedIn(0);
atomic<int> memoryPagesPagedOut(0);

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

void printMemoryState() {
    lock_guard<mutex> coutLock(coutMutex);  // Prevent cout interleaving
    lock_guard<recursive_mutex> memoryLock(memoryMutex);
    cout << "[MEMORY STATE] ";
    for (size_t i = 0; i < memory.size(); ++i) {
        if (memory[i].name == "NULL") {
            cout << "[ ]";
        } else {
            cout << "[" << memory[i].name << "]";
        }
    }
    cout << endl << flush;
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
        memory.resize(max_overall_mem, {"NULL", 0});  // Resize memory to fit the maximum overall memory
        isInitialized = true;
    }
    //printMemoryState();
}


void deallocateMemory(Screen* screen, int mem_per_frame) {
    lock_guard<recursive_mutex> lock(memoryMutex);
    int start = screen->getMemStartIndex();
    int requiredBytes = screen->getRequiredMemory();

    if (start == -1) return; // Already deallocated or never allocated

    std::filesystem::create_directories("backing_store");

    int requiredFrames = requiredBytes / mem_per_frame;

    for (int i = start; i < start + requiredFrames; ++i) {
        if (i >= 0 && i < memory.size()) {
            std::string fileName = "backing_store/" + screen->getName() + "_page" + std::to_string(i - start) + ".txt";
            std::ofstream outFile(fileName);
            if (outFile.is_open()) {
                outFile << "Page: " << (i - start) << "\n";
                outFile << "Memory index: " << i << "\n";
                outFile << "Process: " << screen->getName() << "\n";
                outFile << "Owner: " << memory[i].name << "\n";
                if (memory[i].data.has_value()) {
                    outFile << "Data: " << memory[i].data.value() << "\n";
                } else {
                    outFile << "Data: [none]\n";
                }
                outFile.close();
            }

            // Clear the memory block
            memory[i].name = "NULL";
            memory[i].data.reset();
        }
    }

    screen->setMemStartIndex(-1);

    // Optional debug logging
    // {
    //     lock_guard<mutex> lock(coutMutex);
    //     cout << "[DEALLOCATE] Process " << screen->getName() << " deallocated and written to backing store.\n";
    // }
}


bool allocateMemory(Screen* screen, int mem_per_frame) {
    lock_guard<recursive_mutex> lock(memoryMutex); // Lock memory + FIFO queue

    int memSize = screen->getRequiredMemory();
    int memStart = -1;

    // Step 1: Try to find contiguous free memory
    for (size_t i = 0; i <= memory.size() - memSize; ++i) {
        bool spaceFree = true;
        for (int j = 0; j < memSize; ++j) {
            if (memory[i + j].name != "NULL") {
                spaceFree = false;
                break;
            }
        }
        if (spaceFree) {
            memStart = i;
            break;
        }
    }

    // Step 2: If not enough space, evict FIFO processes until space is available
    while (memStart == -1) {
        if (fifoMemoryQueue.empty()) {
            // cout << "[ALLOCATE-FAIL] Cannot allocate memory for " 
            // << screen->getName() << ". No space and nothing to evict.\n";
            return false; // Cannot evict more; no processes in memory
        }

        // Evict the oldest process in memory (FIFO)
        Screen* evicted = fifoMemoryQueue.front();
        fifoMemoryQueue.pop();

        // {
        //     lock_guard<mutex> lock(coutMutex);
        //     cout << "[EVICT] Evicting process " << evicted->getName()
        //         << " to free up memory.\n";
        // }
        memoryPagesPagedOut++;
        deallocateMemory(evicted, mem_per_frame);
        //printMemoryState();


        // Retry finding space after eviction
        for (size_t i = 0; i <= memory.size() - memSize; ++i) {
            bool spaceFree = true;
            for (int j = 0; j < memSize; ++j) {
                if (memory[i + j].name != "NULL") {
                    spaceFree = false;
                    break;
                }
            }
            if (spaceFree) {
                memStart = i;
                break;
            }
        }
    }

    // Step 3: Allocate memory
    for (int i = memStart; i < memStart + memSize; ++i) {
        memory[i].name = screen->getName();
    }

    screen->setMemStartIndex(memStart);
    memoryPagesPagedIn++; 
    // {
    //     lock_guard<mutex> lock(coutMutex);
    //     cout << "[ALLOCATE] Process " << screen->getName()
    //         << " allocated at index " << memStart
    //         << " for " << memSize << " frames.\n";
    // }


    fifoMemoryQueue.push(screen); // Track this allocation in FIFO order
    // printMemoryState();

    return true;
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
    int batchProcessFreq, int min_ins, int max_ins, int mem_per_frame) {

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

        if (screen == nullptr) {
                totalIdleTicks++;
                totalCpuTicks++;
                this_thread::sleep_for(chrono::milliseconds(10));
                continue;  
            }

        if (screen != nullptr && !screen->isFinished()) {

            if (screen->getMemStartIndex() == -1) {
                if (!allocateMemory(screen, mem_per_frame)) {
                    // Could not allocate memory, requeue and skip
                    lock_guard<mutex> lock(queueMutex);
                    queue.push_back(screen);
                    continue;
                }
            }

            for (int i = 0; i < quantumCycles && !screen->isFinished(); ++i) {
                // {
                //     lock_guard<mutex> lock(coutMutex);
                //     cout << "[RUN] Core " << coreNumber 
                //     << " executing process " << screen->getName() 
                //     << " (Instruction: " << screen->getCurrInstruction() << "/" << screen->getNumInstructions() << ")\n";
                // }
                totalActiveTicks++;
                totalCpuTicks++;

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

                    // File output (Used for Week 10)
                    // string filename = "txt/memory_stamp_core" + to_string(coreNumber) + "_cycle" + to_string(cycleCounter) + ".txt";
                    // ofstream outfile(filename);
                    // if (outfile.is_open()) {
                    //     outfile << "Timestamp: " << getCurrentTime() << "\n";
                    //     outfile << "Core ID: " << coreNumber << "\n";
                    //     outfile << "Quantum Cycle: " << cycleCounter << "\n";
                    //     outfile << "Processes in Memory: " << memCount << "\n";
                    //     outfile << "Total External Fragmentation: " << unusedFrames << " KB\n\n";
                    //     outfile.close();
                    // }
                }
            }

            if (screen->isFinished()) {
                // cout << "[FINISH] Process " << screen->getName() 
                // << " finished execution. Memory will be released.\n";
                deallocateMemory(screen, mem_per_frame);
            } else {
                lock_guard<mutex> lock(queueMutex);
                screen->setRunningToFalse();
                queue.push_back(screen);
                // {
                //     lock_guard<mutex> lock(coutMutex);
                //     cout << "[REQUEUE] Process " << screen->getName() 
                //          << " requeued after quantum.\n";
                // }
            }
        }

        this_thread::sleep_for(chrono::milliseconds(10));
    }
}





void schedulerStart(vector<Screen>& screens, int num_cpu, string scheduler, int quantumCycles, int batchProcessFreq, int min_ins, int max_ins, int mem_per_frame, int mem_per_proc) {
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
            ref(screensMutex), i, quantumCycles, batchProcessFreq, min_ins, max_ins, mem_per_frame);
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

void processSmi(vector<Screen>& screens, int num_cpu) {
    lock_guard<mutex> coutLock(coutMutex);
    lock_guard<recursive_mutex> memoryLock(memoryMutex);

    // ----- CPU UTILIZATION -----
    unordered_set<int> activeCores;
    for (const auto& s : screens) {
        if(s.isRunning()) {
            
            int coreID = s.getAssignedCore();
            if (coreID >= 0){
                activeCores.insert(coreID);
            }
            
        }
    }
    float utilization = (num_cpu > 0) ? (static_cast<float>(activeCores.size()) / num_cpu) * 100.0f : 0.0f;
    cout << "\n---------------------------------------------------------------------------\n";
    cout << "CPU Utilization: " << activeCores.size() << " / " << num_cpu << " cores active (" << utilization << "%)\n";

    // ----- MEMORY USAGE -----
    int usedBytes = 0;
    int totalBytes = memory.size();  // each block is 1 byte
    map<string, int> processMemUsage; // process name -> memory used

    for (const auto& block : memory) {
        if (block.name != "NULL") {
            usedBytes++;
            processMemUsage[block.name]++;
        }
    }

    int memUsagePercent = round((usedBytes * 100.0) / totalBytes);
    cout << "Memory Usage: " << usedBytes << " / " << totalBytes << " bytes (" << memUsagePercent << "%" << " Utilization)" << endl;
    cout << "---------------------------------------------------------------------------\n";

    // ----- RUNNING PROCESSES -----
    if (processMemUsage.empty()) {
        cout << "No running processes in memory." << endl;
    } else {
        cout << "Running Processes and Memory Usage:" << endl;
        for (const auto& [procName, memUsed] : processMemUsage) {
            cout << "  " << procName << ": " << memUsed << " bytes" << endl;
        }
    }
    cout << "---------------------------------------------------------------------------\n\n";

    cout << flush;
}

void vmstat() {
    lock_guard<mutex> lock(coutMutex);
    lock_guard<recursive_mutex> memoryLock(memoryMutex);

    const int frameSizeInBytes = 1; // adjust if 1 frame = N bytes

    int totalMemoryBytes = memory.size() * frameSizeInBytes;
    int usedMemoryBytes = 0;
    int freeMemoryBytes = 0;

    for (const auto& block : memory) {
        if (block.name == "NULL") {
            freeMemoryBytes += frameSizeInBytes;
        } else {
            usedMemoryBytes += frameSizeInBytes;
        }
    }

    int idle = totalIdleTicks.load();
    int active = totalActiveTicks.load();
    int totalTicks = idle + active; // totalCpuTicks.load();

    cout << "[VMSTAT]" << endl;
    cout << "Total Memory: " << totalMemoryBytes << " bytes" << endl;
    cout << "Used Memory: " << usedMemoryBytes << " bytes" << endl;
    cout << "Free Memory: " << freeMemoryBytes << " bytes" << endl;
    cout << "CPU Ticks (Idle): " << idle << endl;
    cout << "CPU Ticks (Active): " << active << endl;
    cout << "CPU Ticks (Total): " << totalTicks << endl;
    cout << "Pages Paged In: " << memoryPagesPagedIn.load() << endl;
    cout << "Pages Paged Out: " << memoryPagesPagedOut.load() << endl;
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
    string fullCommand = command;
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

                } else if (words[1] == "-s" || words[1] == "-c") { // Behavior to create a screen
                    bool exists = false;
                    for (const auto& screen : screens) {
                        if (screen.getName() == words[2]) {
                            cout << "Screen with name '" << words[2] << "' already exists." << "\n";
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        int mem_per_proc_val = mem_per_proc;
                        if (words.size() > 3) {
                            try {
                                mem_per_proc_val = stoi(words[3]);
                            } catch (...) {
                                cout << "Invalid memory per process value. Using initialized value.\n";
                            }
                        }
                        if (isPowerOfTwo(mem_per_proc_val) == false) {
                            cout << "Memory per process must be from 2^6 to 2^16. Using initialized value.\n";
                            mem_per_proc_val = mem_per_proc;
                        }
                        Screen newScreen(words[2], getCurrentTime(), generateInstructions(min_ins, max_ins), mem_per_proc_val);
                        screens.push_back(newScreen);

                        if (words[1] == "-c") {
                            size_t firstQuote = fullCommand.find("\"");
                            size_t lastQuote = fullCommand.find_last_of("\"");

                            if (firstQuote != string::npos && lastQuote != string::npos && lastQuote > firstQuote) {
                                string extracted = fullCommand.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                                screens.back().setCustomInstructions(extracted);
                            } else {
                                cout << "[ERROR] Missing or malformed quoted custom instructions.\n";
                            }
                        }

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

                    cout << "\nUnfinished processes:\n";
                    for (const auto& s : screens) {
                        if(!s.isFinished() && !s.isRunning()) {
                            cout << s.getName() << "    (" << s.getTimeCreated() << ")    " << "Unfinished    " << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
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
                    scheduler, quantumCycles, batchProcessFreq, min_ins, max_ins, mem_per_frame, mem_per_proc);
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
        } else if (words[0] == "process-smi") {
            processSmi(screens, num_cpu);
        } else if (words[0] == "vmstat") {
            vmstat();
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