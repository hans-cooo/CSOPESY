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
