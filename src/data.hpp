#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>

#define DEFAULT_TARGET_CONFIG "ssbs"
#define CMD_BUFFER_SIZE 4096

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
	ERROR
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
	std::filesystem::path filepath;
	std::filesystem::path obj_path;
	std::filesystem::path log_path;
	SourceType type;
	std::vector<LineError> errors;
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

// This is a great resource for various techniques available in g++:
//		https://bytes.usc.edu/cs104/wiki/gcc/

struct BuildResponse
{
	bool succeeded;
	std::vector<Source> sources;
	int errors;
	int warnings;
};

class StupidBuild
{
private:
	std::unordered_map<std::string, std::string> config;
	std::vector<Source> source_files;
	std::vector<Library> libraries;
	std::vector<std::string> library_dirs;
	std::vector<std::string> warnings;
	std::vector<std::string> include_dirs;
	std::string extra_args;
	std::string standard;
	std::string current_dir;
	bool debug_mode = false;
	bool optimize = true;
	bool incremental = true;

	std::filesystem::path get_obj_path(Source source);
	bool has_source_changed(Source source);
	bool load_config(std::string filepath);
	void parse_configs();
	void collect_log_data(Source *source, std::filesystem::path log_file);
	bool init();
	std::vector<Source> get_files_recursive(std::string dir);

public:
	bool open_path(std::string directory);
	BuildResponse build_target(std::string target);
};