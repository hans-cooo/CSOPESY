#include "config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm> // for std::replace
#include <limits>

using namespace std;

bool Config::loadFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open config file.\n";
        return false;
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string key;
        if (!(iss >> key)) continue;

        // Convert hyphens to underscores for compatibility
        replace(key.begin(), key.end(), '-', '_');

        // Get the rest of the line after the key
        string rest_of_line;
        getline(iss, rest_of_line);

        // Trim leading spaces
        rest_of_line.erase(0, rest_of_line.find_first_not_of(" \t"));

        // Remove surrounding quotes if present
        if (!rest_of_line.empty() && rest_of_line.front() == '"' && rest_of_line.back() == '"') {
            rest_of_line = rest_of_line.substr(1, rest_of_line.length() - 2);
        }

        // Assign values
        if (key == "num_cpu") {
            num_cpu = stoi(rest_of_line);
        } else if (key == "scheduler") {
            scheduler = rest_of_line;
        } else if (key == "quantum_cycles") {
            quantumCycles = stoi(rest_of_line);
        } else if (key == "batch_process_freq") {
            batchProcessFreq = stoi(rest_of_line);
        } else if (key == "min_ins") {
            min_ins = stoi(rest_of_line);
        } else if (key == "max_ins") {
            max_ins = stoi(rest_of_line);
        } else if (key == "delay_per_exec") {
            delayPerExec = stoi(rest_of_line);
        } else if (key == "max_overall_mem") {
            max_overall_mem = stoi(rest_of_line);
        } else if (key == "mem_per_frame") {
            mem_per_frame = stoi(rest_of_line);
        } else if (key == "max_mem_per_proc") {
            mem_per_proc = stoi(rest_of_line);
        } else {
            // cerr << "Unknown config key: " << key << "\n";
            file.ignore(numeric_limits<streamsize>::max(), '\n'); // skip rest of line
        }
    }

    return true;
}

void Config::print() const {
    cout << "Configuration Loaded:\n";
    cout << "num_cpu: " << num_cpu << "\n";
    cout << "scheduler: " << scheduler << "\n";
    cout << "quantumCycles: " << quantumCycles << "\n";
    cout << "batchProcessFreq: " << batchProcessFreq << "\n";
    cout << "min_ins: " << min_ins << "\n";
    cout << "max_ins: " << max_ins << "\n";
    cout << "delayPerExec: " << delayPerExec << "\n";
    cout << "max_overall_mem: " << max_overall_mem << "\n";
    cout << "mem_per_frame: " << mem_per_frame << "\n";
    cout << "mem_per_proc: " << mem_per_proc << "\n";
}
