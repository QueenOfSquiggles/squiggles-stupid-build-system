#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

#define DEFAULT_TARGET_CONFIG "ssbs"
#define CMD_BUFFER_SIZE 4096

struct KeyValuePair
{
	std::string key;
	std::string value;
};

enum SourceType
{
	CPP_SOURCE,
	HPP_SOURCE,
	C_SOURCE,
	H_SOURCE,
};

struct Source
{
	fs::path filepath;
	SourceType type;
};

enum LinkMode
{
	STATIC,
	DYNAMIC
};

struct Library
{
	std::string name;
	LinkMode link;
};

struct LineError
{
	std::string file;
	int line;
	std::string message;
};

struct BuildResponse
{
	bool succeeded;
	std::vector<LineError> errors;
	std::vector<LineError> warnings;
};

class StupidBuild
{
private:
	std::unordered_map<std::string, std::string> config;
	std::vector<Source> source_files;
	std::vector<Library> libraries;
	std::vector<std::string> warnings;
	std::vector<std::string> include_dirs;
	std::string extra_args;
	std::string standard;
	std::string current_dir;
	bool load_config(std::string filepath);
	void parse_configs();
	bool init();
	std::vector<Source> get_files_recursive(std::string dir);

public:
	bool open_path(std::string directory);
	BuildResponse build_target(std::string target);
};