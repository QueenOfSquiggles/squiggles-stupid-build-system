#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <iostream>

#define DEFAULT_TARGET_CONFIG "ssbs"

struct Source;
struct BuildResponse
{
	bool succeeded;
	std::vector<Source> sources;
	int errors;
	int warnings;
};

enum SourceType
{
	CPP_SOURCE,
	HPP_SOURCE,
	C_SOURCE,
	H_SOURCE,
};

enum ErrorType
{
	NOTE,
	WARNING,
	ERROR,
	INCLUDE_ERROR
};

struct LineError
{
	int line;
	int column;
	ErrorType type;
	std::string message;
};

struct Source
{
	SourceType type;
	std::filesystem::path filepath;
	std::filesystem::path obj_path;
	std::filesystem::path log_path;
	std::vector<LineError> errors;
	std::vector<LineError> notes;
	std::vector<LineError> warnings;
};

enum LinkMode
{ // TODO implement linking modes
	STATIC,
	DYNAMIC
};

struct Library
{
	std::string name;
	LinkMode link;
};

class Config
{

public:
	Config();
	Config(std::string target_config);
	std::map<std::string, std::string> config;
	std::vector<Source> source_files;
	std::vector<Library> libraries;
	std::vector<std::string> library_dirs;
	std::vector<std::string> warnings;
	std::vector<std::string> include_dirs;
	std::string extra_args;
	std::string standard;
	std::string current_dir;
	std::string compiler;
	std::string bin;
	bool debug_mode = false;
	bool optimize = true;
	bool incremental = true;

	bool has_source_changed(Source source);
	void collect_log_data(Source *source, std::filesystem::path log_file);
};