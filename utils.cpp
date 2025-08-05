#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include "screen.h"
#include "utils.h"

using namespace std;

std::vector<std::string> split_sentence(std::string sen) {
    std::vector<std::string> words;
    std::string word = "";

    for (char c : sen) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }

    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

string getCurrentTime() {
    auto now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now);
    tm *now_tm = localtime(&now_c);

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S %p", now_tm);
    return string(buffer);
}

void clearScreen() {
    cout << "\033[2J\033[1;1H"; // ANSI escape code to clear the screen
}

void reportUtilToFile(const vector<Screen>& screens, const string& filename, int num_cpu) {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Failed to open report file for writing.\n";
        return;
    }

    unordered_set<int> activeCores;

    out << "---------------------------------------------------------------------------\n";
    out << "Running processes:\n";
    for (const auto& s : screens) {
        if (s.isRunning()) {
            out << s.getName() << "    (" << s.getTimeCreated() << ")    "
                << "Core: " << s.getAssignedCore() << "    "
                << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";

            int coreID = s.getAssignedCore();
            if (coreID >= 0) {
                activeCores.insert(coreID);
            }
        }
    }

    out << "\nFinished processes:\n";
    for (const auto& s : screens) {
        if (s.isFinished()) {
            out << s.getName() << "    (" << s.getTimeCreated() << ")    "
                << "Finished    " << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
        }
    }

    // Calculate CPU Utilization
    float utilization = (num_cpu > 0) ? (static_cast<float>(activeCores.size()) / num_cpu) * 100.0f : 0.0f;
    out << "\nCPU Utilization: " << activeCores.size() << " / " << num_cpu << " cores active (" << utilization << "%)\n";

    out << "---------------------------------------------------------------------------\n";

    out.close();
}

// READ: Returns the uint16_t value at memory[address]
uint16_t READ(const std::string& varName, int address) {
    if (address < 0 || address >= memory.size()) {
        cerr << "[READ] Error: Address out of bounds: " << address << endl;
        return 0;
    }

    if (memory[address].data.has_value()) {
        return memory[address].data.value();
    } else {
        return 0;
    }
}

// WRITE: Sets memory[address].data to value
void WRITE(int address, uint16_t value) {
    if (address < 0 || address >= memory.size()) {
        cerr << "[WRITE] Error: Address out of bounds: " << address << endl;
        return;
    }

    memory[address].data = value;
}