#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include "configuration/config.hpp"

class CMakeGenerator
{
	static void write_cmake(Config config);

public:
	static bool generate_cmake_config(std::string configuration, std::vector<std::string> flags);
};