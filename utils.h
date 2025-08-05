#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <chrono>

extern std::vector<MemoryBlock> memory;

std::vector<std::string> split_sentence(std::string sen);
std::string getCurrentTime();
void clearScreen();
void reportUtilToFile(const vector<Screen>& screens, const string& filename, int num_cpu);
bool isPowerOfTwo(int n);
int hexToDecimal(const std::string& hexStr);

uint16_t READ(const std::string& varName, size_t memoryAddress);
void WRITE(size_t memoryAddress, uint16_t value);
#endif
