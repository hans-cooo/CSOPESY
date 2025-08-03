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

bool isPowerOfTwo(int n) {
    return n > 0 && (n & (n - 1)) == 0;
}

int hexToDecimal(const std::string& hexStr) {
    if (hexStr.substr(0, 2) != "0x" && hexStr.substr(0, 2) != "0X") {
        std::cerr << "Invalid hex format\n";
        return -1;
    }

    try {
        return std::stoi(hexStr, nullptr, 16);
    } catch (const std::exception& e) {
        std::cerr << "Conversion error: " << e.what() << '\n';
        return -1;
    }
}
