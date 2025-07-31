#ifndef CONFIG_H
#define CONFIG_H

#include <string>

class Config {
public:
    int num_cpu = 0;
    std::string scheduler;
    int quantumCycles = 0;
    int batchProcessFreq = 0;
    int min_ins = 0;
    int max_ins = 0;
    int delayPerExec = 0;
    int max_overall_mem = 0;
    int mem_per_frame = 0;
    int mem_per_proc = 0;

    bool loadFromFile(const std::string& filename);
    void print() const;
};

#endif
