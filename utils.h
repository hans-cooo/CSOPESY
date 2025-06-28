#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <chrono>

std::vector<std::string> split_sentence(std::string sen);
std::string getCurrentTime();
void clearScreen();
void reportUtilToFile(const vector<Screen>& screens, const string& filename);



#endif
