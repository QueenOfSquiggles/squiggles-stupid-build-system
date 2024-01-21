#include <fstream>
#include <stdio.h>
#include <iostream>
#include <filesystem>

#include "data.hpp"

using namespace std;

bool StupidBuild::load_config(string filepath)
{

	ifstream reader;
	string n_path = this->current_dir + "/" + filepath + ".cfg";
	reader.open(n_path);
	if (!reader.is_open())
	{
		cerr << "Failed to open configuration target: [" << n_path << "]" << endl;
		return false;
	}
	string line;
	while (getline(reader, line))
	{
		int comment_idx = line.find_first_of('#');
		if (comment_idx != -1)
		{
			line = line.substr(0, comment_idx);
		}
		int idx = line.find_first_of('=');
		if (line.length() <= 0 || idx == -1)
		{
			continue;
		}
		if (line.find_first_of(",") != -1)
		{
			cerr << "Please omit ',' from your configuration files!!" << endl;
		}
		string key = line.substr(0, idx);
		string value = line.substr(idx + 1, line.length());
		this->config[key] = value;
	}
	reader.close();
	return true;
}

void StupidBuild::parse_configs()
{
	// TODO: parse more complex configurations into the class's values
	if (auto s = this->config.find("std"); s != this->config.end())
	{
		this->standard = s->second;
	}
	if (auto s = this->config.find("extra"); s != this->config.end())
	{
		this->extra_args = s->second;
	}
}

bool StupidBuild::init()
{
	ofstream writer;
	string n_path = this->current_dir + "/" + DEFAULT_TARGET_CONFIG + ".cfg";
	writer.open(n_path);
	if (!writer.is_open())
	{
		return false;
	}
	writer << "# This is a comment line" << endl;
	writer << "# This file is your default configuration" << endl;
	writer << "# Any other *.cfg files in this directory will serve as build targets" << endl;
	writer << endl;
	writer << endl;
	writer << "binary=main" << endl;
	writer << "source=src" << endl;
	writer << "warnings=[ all ]" << endl;
	writer << "# libs=[ qt opengl raylib ]" << endl;
	writer << "# include=[ /path/to/custom/include/dir ./path/to/local/include ]" << endl;
	return true;
}

vector<Source> StupidBuild::get_files_recursive(string dir)
{
	vector<Source> files;
	vector<filesystem::path> dirs;
	vector<filesystem::path> g_files;
	for (const auto &entry : filesystem::directory_iterator(dir))
	{
		if (entry.is_directory())
		{
			dirs.push_back(entry.path());
		}
		else
		{
			g_files.push_back(entry.path());
		}
	}
	for (filesystem::path d : dirs)
	{
		vector<Source> n_files = get_files_recursive(d.generic_string());
		for (Source f : n_files)
		{
			files.push_back(f);
		}
	}
	for (filesystem::path f : g_files)
	{
		Source source;
		source.filepath = f;
		string ext = f.extension();

		if (ext.compare(".cpp") == 0)
		{
			source.type = SourceType::CPP_SOURCE;
		}
		else if (ext.compare(".hpp") == 0)
		{
			source.type = SourceType::HPP_SOURCE;
		}
		else if (ext.compare(".c") == 0)
		{
			source.type = SourceType::C_SOURCE;
		}
		else if (ext.compare(".h") == 0)
		{
			source.type = SourceType::H_SOURCE;
		}
		else
		{
			continue;
		}
		files.push_back(source);
	}
	return files;
}

bool StupidBuild::open_path(string directory)
{
	this->current_dir = directory;
	if (!this->load_config(DEFAULT_TARGET_CONFIG))
	{
		if (!this->init())
		{
			cerr << "failed to write to default target config file!!";
		}
		return false;
	}
	filesystem::path p(directory);
	p.append(this->config["source"]);
	this->source_files = get_files_recursive(p.generic_string());

	return true;
}

vector<string> tokenize(string input)
{
	stringstream stream(input);
	vector<string> tokens;
	string buffer;
	while (getline(stream, buffer, ' '))
	{
		tokens.push_back(buffer);
	}
	return tokens;
}

BuildResponse StupidBuild::build_target(string target)
{
	if (target.compare(DEFAULT_TARGET_CONFIG) != 0)
	{
		this->load_config(target);
	}
	BuildResponse resp;
	string bin = this->config["binary"];
	// construct arguments chain
	string args;
	for (string incl : this->include_dirs)
	{
		args += " -I" + incl;
	}
	for (string warning : this->warnings)
	{
		args += " -W" + warning;
	}
	if (!this->standard.empty())
	{
		args += " -std=" + this->standard;
	}
	if (!this->extra_args.empty())
	{
		args += this->extra_args;
	}

	// build object files
	for (Source s : this->source_files)
	{
		if (s.type == SourceType::H_SOURCE || s.type == SourceType::HPP_SOURCE)
		{
			continue;
		}

		filesystem::path rel_path = filesystem::relative(s.filepath);

		char cmd[CMD_BUFFER_SIZE];
		snprintf(cmd, CMD_BUFFER_SIZE, "g++ -c %s %s -o ./build/temp/%s",
				 args.c_str(),
				 rel_path.generic_string().c_str(),
				 (rel_path.stem().string() + ".o").c_str());
		FILE *stream = popen(cmd, "r");
		char line[CMD_BUFFER_SIZE];
		while (fgets(line, CMD_BUFFER_SIZE, stream) != NULL)
		{
			printf("[%s] (%s) :: %s \n", bin.c_str(), rel_path.filename(), line);
		}

		if (stream == NULL)
		{
			printf("Failed to assemble source file at %s \n", rel_path.generic_string().c_str());
			continue;
		}
	}

	// compile/link binary
	char cmd[CMD_BUFFER_SIZE];
	snprintf(cmd, CMD_BUFFER_SIZE, "g++ -o ./build/%s ./build/temp/*.o", bin.c_str());
	FILE *stream = popen(cmd, "r");
	char line[CMD_BUFFER_SIZE];
	while (fgets(line, CMD_BUFFER_SIZE, stream) != NULL)
	{
		printf("[%s] (*) :: %s \n", bin.c_str(), line);
	}

	resp.succeeded = resp.warnings.size() == 0 && resp.errors.size() == 0;
	return resp;
}