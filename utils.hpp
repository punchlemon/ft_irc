#pragma once
#include <string>
#include <map>
#include <stdexcept>

std::pair<int, std::string> validateInput(const int& argc, const char**& argv);

bool isValidChannelName(const std::string& name);
