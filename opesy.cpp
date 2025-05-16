#include <iostream>

void printHeader() {
    std::cout << "   ____   ____    _____   ____    ____   ____   __   __" << "\n";
    std::cout << "  / ___/ / ___|  /  _  | |  _ \\  |  __/ / ___|  \\ \\ / /" << "\n";
    std::cout << " | |     | |__   | | | | | |_| | | |_   | |__    \\ V / " << "\n";
    std::cout << " | |     \\___ \\  | | | | |  __/  |  _|  \\___ \\    | |  " << "\n";
    std::cout << " | |____  ___| | | |_| | | |     | |__   ___| |   | |  " << "\n";
    std::cout << "  \\____| |____/  |____/  |_|     |____| |____/    |_|  " << "\n";

    std::cout << "Hello, Welcome to CSOPESY commandline!" << "\n";
    std::cout << "Type 'exit' to quit, and 'clear' to clear the screen" << "\n";
}

void initialize() {
    std::cout << "initialize command recognized. Doing Something." << "\n";
}

void screen() {
    std::cout << "screen command recognized. Doing Something." << "\n";
}

void schedulerTest() {
    std::cout << "scheduler-test command recognized. Doing Something." << "\n";
}

void schedulerStop() {
    std::cout << "scheduler-stop command recognized. Doing Something." << "\n";
}

void reportUtil() {
    std::cout << "report-util command recognized. Doing Something." << "\n";
}

int main() {
    std::string command;
    printHeader();
    std::cout << "Enter a command: ";
    std::cin >> command;

    while (command != "exit") {
        if (command == "clear") {
            std::cout << "\033[2J\033[1;1H"; // ANSI escape code to clear the screen
            printHeader();
        } else if (command == "initialize") {
            initialize();
        } else if (command == "screen") {
            screen();
        } else if (command == "scheduler-test") {
            schedulerTest();
        } else if (command == "scheduler-stop") {
            schedulerStop();
        } else if (command == "report-util") {
            reportUtil();
        } else {
            std::cout << "Invalid command." << "\n";
        }
        std::cout << "Enter a command: ";
        std::cin >> command;
    }
    return 0;
}