#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdio.h>

#include "data.hpp"

using namespace std;

vector<string> tokenize(string input);

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
		size_t comment_idx = line.find_first_of('#');
		if (comment_idx != string::npos)
		{
			line = line.substr(0, comment_idx);
		}
		size_t idx = line.find_first_of('=');
		if (line.length() <= 0u || idx == string::npos)
		{
			continue;
		}
		if (line.find_first_of(",") != string::npos)
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
	if (auto s = this->config.find("std"); s != this->config.end())
	{
		this->standard = s->second;
	}
	if (auto s = this->config.find("extra"); s != this->config.end())
	{
		this->extra_args = s->second;
	}
	if (auto s = this->config.find("compiler"); s != this->config.end())
	{
		this->compiler = s->second;
	}
	if (auto s = this->config.find("debug"); s != this->config.end())
	{
		this->debug_mode = s->second.compare("true") == 0;
	}
	if (auto s = this->config.find("optimize"); s != this->config.end())
	{
		this->optimize = s->second.compare("true") == 0;
	}
	if (auto s = this->config.find("incremental"); s != this->config.end())
	{
		this->incremental = s->second.compare("true") == 0;
	}
	if (auto s = this->config.find("warnings"); s != this->config.end())
	{
		auto tokens = tokenize(s->second);
		for (string t : tokens)
		{
			if (t[0] == '[' || t[0] == ']')
			{
				continue;
			}
			this->warnings.push_back(t);
		}
	}
	if (auto s = this->config.find("libs"); s != this->config.end())
	{
		auto tokens = tokenize(s->second);
		for (string t : tokens)
		{
			if (t[0] == '[' || t[0] == ']')
			{
				continue;
			}
			Library lib;
			lib.name = t;
			lib.link = LinkMode::STATIC;
			this->libraries.push_back(lib);
		}
	}
	if (auto s = this->config.find("include"); s != this->config.end())
	{
		auto tokens = tokenize(s->second);
		for (string t : tokens)
		{
			if (t[0] == '[' || t[0] == ']')
			{
				continue;
			}
			this->include_dirs.push_back(t);
		}
	}
	if (auto s = this->config.find("libdirs"); s != this->config.end())
	{
		auto tokens = tokenize(s->second);
		for (string t : tokens)
		{
			if (t[0] == '[' || t[0] == ']')
			{
				continue;
			}
			this->library_dirs.push_back(t);
		}
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
	writer
		<< "# Any other *.cfg files in this directory will serve as build targets"
		<< endl;
	writer << endl;
	writer << endl;
	writer << "binary=main" << endl;
	writer << "source=src" << endl;
	writer << "warnings=[ all ]" << endl;
	writer << "# libs=[ qt opengl raylib ]" << endl;
	writer << "# include=[ /path/to/custom/include/dir ./path/to/local/include ]"
		   << endl;
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
		string ext = f.extension().string();

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
		auto global_path = f.parent_path().string();
		auto index = global_path.find(this->config["source"]);
		auto offset = this->config["source"].length() + 1;
		string dir = "";
		if (index + offset < global_path.length())
		{
			dir = global_path.substr(index + offset);
		}

		filesystem::path obj_path = filesystem::relative("./build/obj");
		obj_path.append(dir);
		obj_path.append(f.stem().string() + ".o");
		source.obj_path = obj_path;

		filesystem::path log_path = filesystem::relative("./build/log");
		log_path.append(dir);
		log_path.append(f.stem().string() + ".log.txt");
		source.log_path = log_path;
		// cout << "Source file: " << f << endl
		// 	 << "\tOBJ: " << obj_path << endl
		// 	 << "\tLOG: " << log_path << endl;
		files.push_back(source);
	}
	return files;
}
void StupidBuild::collect_log_data(Source *source, std::filesystem::path log_file)
{
	// read file
	ifstream reader(log_file);
	if (!reader.is_open())
	{
		cerr << "Failed to read log file for " << log_file << endl;
		return;
	}

	string line = "";
	int log_file_line = 0;
	int line_num = 0;
	int column = 0;
	string msg_type = "";
	bool handling_include_error = false;
	while (getline(reader, line))
	{
		log_file_line++;
		if (!handling_include_error)
		{ // handle typical source file-centric errors/warnings
			if (line.substr(0, 4).compare("src/") != 0 ||
				line.find(":") == string::npos)
			{
				continue;
			}
			// source file
			size_t idx = line.find(":");
			line = line.substr(idx + 1);
			// line number
			idx = line.find(':');
			if (idx > 8u || idx == string::npos)
			{
				continue;
			}
			line_num = stoi(line.substr(0, idx));
			line = line.substr(idx + 1);

			// column number
			idx = line.find(":");
			column = stoi(line.substr(0, idx));
			line = line.substr(idx + 1);

			// message type
			idx = line.find(":");
			if (idx == string::npos)
			{
				// try find include error
				idx = line.find_first_not_of(' ');
				string check = line.substr(idx);
				// std::cout << "Checking \"" << check << "\" for include error key words" << std::endl;
				if (check.compare("required from here") == 0)
				{
					handling_include_error = true;
					continue;
				}
			}
			msg_type = line.substr(1, idx - 1); // remove space
			line = line.substr(idx + 1);
		}
		else
		{ // handle include related error
			size_t idx;
			for (int i = 0; i < 3; i++)
			{ // clear include file paths
				idx = line.find(":");
				line = line.substr(idx + 1);
			}
			idx = line.find(":");
			msg_type = "note";
			if (idx != string::npos)
			{
				msg_type = line.substr(1, idx - 1); // remove space
				line = line.substr(idx + 1);
			}

			// std::cout << "Handling include error: \"" << line << "\"" << std::endl
			// 		  << "\tType: \"" << msg_type << "\"" << std::endl;
		}

		// message
		LineError err;
		err.line = line_num;
		err.column = column;
		err.message = line + "\n[" + log_file.string() + ":" +
					  to_string(log_file_line) + "]";
		// cout << "Message: '" << line << "'" << endl
		//  << endl;
		if (handling_include_error)
		{
			// err.type = ErrorType::INCLUDE_ERROR;
			// source->errors.push_back(err);
			handling_include_error = false;
		}
		if (msg_type.compare("error") == 0)
		{
			err.type = ErrorType::ERROR;
			source->errors.push_back(err);
		}
		else if (msg_type.compare("note") == 0)
		{
			err.type = ErrorType::NOTE;
			source->notes.push_back(err);
		}
		else
		{
			err.type = ErrorType::WARNING;
			source->warnings.push_back(err);
		}
	}
}

bool StupidBuild::open_path(string directory)
{
	this->current_dir = directory;
	compiler = "g++";
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

bool StupidBuild::has_source_changed(Source source)
{
	if (!filesystem::exists(source.obj_path))
	{
		return true;
	}
	return filesystem::last_write_time(source.filepath) >
		   filesystem::last_write_time(source.obj_path);
}

BuildResponse StupidBuild::build_target(string target)
{
	if (target.compare(DEFAULT_TARGET_CONFIG) != 0)
	{
		this->load_config(target);
	}
	this->parse_configs();
	BuildResponse resp;
	string bin = this->config["binary"];
	// construct arguments chain
	string args;

	if (this->debug_mode)
	{
		args += " -g";
	}
	for (string incl : this->include_dirs)
	{
		args += " -I ./" + incl;
	}
	// include source dir automatically
	args += " -I ./src/";
	for (string dir : this->library_dirs)
	{
		args += " -L ./" + dir;
	}
	for (string warning : this->warnings)
	{
		args += " -W" + warning;
	}
	for (Library lib : this->libraries)
	{
		args += " -l" + lib.name;
	}
	if (!this->standard.empty())
	{
		args += " -std=" + this->standard;
	}
	if (this->optimize)
	{
		args += " -O2";
	}
	if (!this->extra_args.empty())
	{
		args += this->extra_args;
	}
	string object_sources = "";
	int index = 0;
	auto targets = std::vector<Source>();
	for (Source s : this->source_files)
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

		if (!this->incremental || has_source_changed(s))
		{
			char cmd[CMD_BUFFER_SIZE];
			snprintf(cmd, CMD_BUFFER_SIZE, "%s -c %s %s -o %s &> %s",
					 compiler.c_str(), args.c_str(),
					 rel_path.generic_string().c_str(), s.obj_path.c_str(),
					 s.log_path.c_str());
			std::cout << "[o " << (++index) << "/" << std::to_string(targets.size())
					  << "]  " << cmd << std::endl;
			system(cmd);
		}
		else
		{
			std::cout << "[o " << (++index) << "/" << std::to_string(targets.size())
					  << "]  skipped (" << rel_path.string() << ")" << std::endl;
		}
		collect_log_data(&s, s.log_path);
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
			 compiler.c_str(), bin.c_str(), args.c_str(), object_sources.c_str(),
			 (bin + ".log.txt").c_str());
	int result = system(cmd);
	std::cout << "[bin] " << cmd << std::endl;
	if (result != 0)
	{
		cerr << "Failed to link binary. See " << bin_log.string()
			 << " for more details" << endl;
		resp.succeeded = false;
	}
	return resp;
}
