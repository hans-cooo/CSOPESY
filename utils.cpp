#include <fstream>
#include <vector>
#include <string>
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

void reportUtilToFile(const vector<Screen>& screens, const string& filename = "report.txt") {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Failed to open report file for writing.\n";
        return;
    }

    out << "---------------------------------------------------------------------------\n";
    out << "Running processes:\n";
    for (const auto& s : screens) {
        if (s.isRunning()) {
            out << s.getName() << "    (" << s.getTimeCreated() << ")    "
                << "Core: " << s.getAssignedCore() << "    "
                << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
        }
    }

    out << "\nFinished processes:\n";
    for (const auto& s : screens) {
        if (s.isFinished()) {
            out << s.getName() << "    (" << s.getTimeCreated() << ")    "
                << "Finished    " << s.getCurrInstruction() << " / " << s.getNumInstructions() << "\n";
        }
    }

    out << "---------------------------------------------------------------------------\n";

    out.close();
}
