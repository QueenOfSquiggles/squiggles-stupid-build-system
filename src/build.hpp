#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include "configuration/config.hpp"

#define CMD_BUFFER_SIZE 4096

// This is a great resource for various techniques available in g++:
//		https://bytes.usc.edu/cs104/wiki/gcc/

class StupidBuild
{
	Config config;

public:
	bool open_path(std::string directory);
	BuildResponse build_target(std::string target);
};