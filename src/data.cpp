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
		filesystem::path obj_path = filesystem::relative("build/temp");
		obj_path.append(f.stem().string() + ".o");
		source.obj_path = obj_path;

		filesystem::path log_path = filesystem::relative("build/temp");
		log_path.append(f.stem().string() + ".log.txt");
		source.log_path = log_path;

		files.push_back(source);
	}
	return files;
}
void StupidBuild::collect_log_data(Source *source,
								   std::filesystem::path log_file)
{
	// read file
	ifstream reader(log_file);
	if (!reader.is_open())
	{
		cerr << "Failed to read log file for " << log_file << endl;
		return;
	}

	string line;
	int log_file_line = 0;
	while (getline(reader, line))
	{
		log_file_line++;
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
		// cout << "Error from " << file << ":" << log_file_line << endl;
		// cout << "Line: <<" << line << ">>" << endl;
		// cout << "File: " << source << endl;
		// cout << "Line Number: '" << line.substr(0, idx) << "'" << endl;
		int line_num = stoi(line.substr(0, idx));
		line = line.substr(idx + 1);

		// column number
		idx = line.find(":");
		// cout << "Line Column: '" << line.substr(0, idx) << "'" << endl;
		int column = stoi(line.substr(0, idx));
		line = line.substr(idx + 1);

		// message type
		idx = line.find(":");
		string msg_type = line.substr(1, idx - 1); // remove space
		// cout << "Type: '" << msg_type << "'" << endl;
		line = line.substr(idx + 1);

		// message
		LineError err;
		err.line = line_num;
		err.column = column;
		err.message = line;
		// cout << "Message: '" << line << "'" << endl
		//  << endl;

		if (msg_type.compare("error") == 0)
		{
			err.type = ErrorType::ERROR;
			source->errors.push_back(err);
		}
		else if (msg_type.compare("note") == 0)
		{
			err.type = ErrorType::NOTE;
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

filesystem::path StupidBuild::get_obj_path(Source source)
{
	filesystem::path path = filesystem::relative("./build/temp");
	path.append(source.filepath.stem().string() + ".o");
	return path;
}

bool StupidBuild::has_source_changed(Source source)
{
	if (!filesystem::exists(this->get_obj_path(source)))
	{
		return true;
	}
	return filesystem::last_write_time(source.filepath) >
		   filesystem::last_write_time(this->get_obj_path(source));
}

BuildResponse StupidBuild::build_target(string target)
{
	// make build dirs
	filesystem::create_directories("./build/temp");

	if (target.compare(DEFAULT_TARGET_CONFIG) != 0)
	{
		this->load_config(target);
	}
	this->parse_configs();
	BuildResponse resp;
	string bin = this->config["binary"];
	// construct arguments chain
	string args;

	for (string incl : this->include_dirs)
	{
		args += " -I /$(pwd)/" + incl;
	}
	for (string dir : this->library_dirs)
	{
		args += " -L /$(pwd)/" + dir;
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
	if (this->debug_mode)
	{
		args += " -g";
	}
	if (this->optimize)
	{
		args += " -O2";
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
		if (!this->incremental || has_source_changed(s))
		{
			char cmd[CMD_BUFFER_SIZE];
			snprintf(cmd, CMD_BUFFER_SIZE, "g++ -c %s %s -o %s &> %s", args.c_str(),
					 rel_path.generic_string().c_str(), get_obj_path(s).c_str(),
					 s.log_path.c_str());
			system(cmd);
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
	snprintf(cmd, CMD_BUFFER_SIZE,
			 "g++ -o ./build/%s %s ./build/temp/*.o &> ./build/%s", bin.c_str(),
			 args.c_str(), (bin + ".log.txt").c_str());
	if (system(cmd) != 0)
	{
		cerr << "Failed to link binary. See " << bin_log.string()
			 << " for more details" << endl;
		resp.succeeded = false;
	}
	return resp;
}
