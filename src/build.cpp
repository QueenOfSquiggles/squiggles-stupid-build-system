#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdio.h>

#include "build.hpp"

using namespace std;

BuildResponse StupidBuild::build_target(string target)
{
	config = Config(target);
	BuildResponse resp;
	string bin = config.bin;
	// construct arguments chain
	string args;

	if (config.debug_mode)
	{
		args += " -g";
	}
	for (string incl : config.include_dirs)
	{
		args += " -I " + incl;
	}

	for (string dir : config.library_dirs)
	{
		args += " -L ./" + dir;
	}
	for (string warning : config.warnings)
	{
		args += " -W" + warning;
	}
	for (Library lib : config.libraries)
	{
		args += " -l" + lib.name;
	}
	if (!config.standard.empty())
	{
		args += " -std=c++" + config.standard;
	}
	if (config.optimize)
	{
		args += " -O2";
	}
	if (!config.extra_args.empty())
	{
		args += config.extra_args;
	}
	string object_sources = "";
	int index = 0;
	auto targets = std::vector<Source>();
	for (Source s : config.source_files)
	{
		if (s.type == SourceType::H_SOURCE || s.type == SourceType::HPP_SOURCE)
		{
			continue;
		}
		targets.push_back(s);
	}
	// build object files
	for (Source s : targets)
	{
		filesystem::create_directories(s.log_path.parent_path());
		filesystem::create_directories(s.obj_path.parent_path());

		filesystem::path rel_path = filesystem::relative(s.filepath);
		object_sources += filesystem::relative(s.obj_path).string() + " ";

		if (!config.incremental || config.has_source_changed(s))
		{
			char cmd[CMD_BUFFER_SIZE];
			snprintf(cmd, CMD_BUFFER_SIZE, "%s -c %s %s -o %s &> %s",
					 config.compiler.c_str(), args.c_str(),
					 rel_path.generic_string().c_str(), s.obj_path.c_str(),
					 s.log_path.c_str());
			std::cout << "[o " << (++index) << "/" << std::to_string(targets.size())
					  << "]\t\t" << cmd << std::endl;
			system(cmd);
		}
		else
		{
			std::cout << "[o " << (++index) << "/" << std::to_string(targets.size())
					  << "]\t\tskipped (" << rel_path.string() << ")" << std::endl;
		}
		config.collect_log_data(&s, s.log_path);
		resp.sources.push_back(s);
		resp.errors += s.errors.size();
		resp.warnings += s.warnings.size();
	}
	resp.succeeded = resp.errors == 0;
	filesystem::path bin_log = filesystem::relative("./build");
	bin_log.append(bin + ".log.txt");
	// compile/link binary
	char cmd[CMD_BUFFER_SIZE];
	snprintf(cmd, CMD_BUFFER_SIZE, "%s -o ./build/%s %s %s &> ./build/%s",
			 config.compiler.c_str(), bin.c_str(), args.c_str(), object_sources.c_str(),
			 (bin + ".log.txt").c_str());
	int result = system(cmd);
	std::cout << "[bin]\t\t" << cmd << std::endl;
	if (result != 0)
	{
		cerr << "Failed to link binary. See " << bin_log.string()
			 << " for more details" << endl;
		resp.succeeded = false;
	}
	return resp;
}
