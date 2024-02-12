#include "config.hpp"

using namespace std;

vector<string> tokenize(string input, Config &config);

bool load_config(string filepath, Config &config)
{

	ifstream reader;
	reader.open(filepath);
	if (!reader.is_open())
	{
		cerr << "Failed to open configuration target: [" << filepath << "]" << endl;
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
		config.config[key] = value;
	}
	reader.close();
	return true;
}

void parse_configs(Config &config)
{
	// defaults
	config.bin = "main";
	config.include_dirs.push_back("./src/");
	if (auto s = config.config.find("std"); s != config.config.end())
	{
		config.standard = s->second;
	}
	if (auto s = config.config.find("extra"); s != config.config.end())
	{
		config.extra_args = s->second;
	}
	if (auto s = config.config.find("compiler"); s != config.config.end())
	{
		config.compiler = s->second;
	}
	if (auto s = config.config.find("bin"); s != config.config.end())
	{
		config.bin = s->second;
	}
	if (auto s = config.config.find("debug"); s != config.config.end())
	{
		config.debug_mode = s->second.compare("true") == 0;
	}
	if (auto s = config.config.find("optimize"); s != config.config.end())
	{
		config.optimize = s->second.compare("true") == 0;
	}
	if (auto s = config.config.find("incremental"); s != config.config.end())
	{
		config.incremental = s->second.compare("true") == 0;
	}
	if (auto s = config.config.find("warnings"); s != config.config.end())
	{
		auto tokens = tokenize(s->second, config);
		for (string t : tokens)
		{
			if (t[0] == '[' || t[0] == ']')
			{
				continue;
			}
			config.warnings.push_back(t);
		}
	}
	if (auto s = config.config.find("libs"); s != config.config.end())
	{
		auto tokens = tokenize(s->second, config);
		for (string t : tokens)
		{
			if (t[0] == '[' || t[0] == ']')
			{
				continue;
			}
			Library lib;
			lib.name = t;
			lib.link = LinkMode::STATIC;
			config.libraries.push_back(lib);
		}
	}
	if (auto s = config.config.find("include"); s != config.config.end())
	{
		auto tokens = tokenize(s->second, config);
		for (string t : tokens)
		{
			if (t[0] == '[' || t[0] == ']')
			{
				continue;
			}
			config.include_dirs.push_back(t);
		}
	}
	if (auto s = config.config.find("libdirs"); s != config.config.end())
	{
		auto tokens = tokenize(s->second, config);
		for (string t : tokens)
		{
			if (t[0] == '[' || t[0] == ']')
			{
				continue;
			}
			config.library_dirs.push_back(t);
		}
	}
}

vector<Source> get_files_recursive(string dir, Config &config)
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
		vector<Source> n_files = get_files_recursive(d.generic_string(), config);
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
		auto index = global_path.find(config.config["source"]);
		auto offset = config.config["source"].length() + 1;
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
		files.push_back(source);
	}
	return files;
}
void Config::collect_log_data(Source *source, std::filesystem::path log_file)
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

bool open_path(string directory, Config &config)
{
	config.current_dir = directory;
	config.compiler = "g++";
	auto path = filesystem::path(directory);
	path.append(string(DEFAULT_TARGET_CONFIG) + ".cfg");
	if (!load_config(path.string(), config))
	{
		return false;
	}
	filesystem::path p(directory);
	p.append(config.config["source"]);
	config.source_files = get_files_recursive(p.generic_string(), config);

	return true;
}

vector<string> tokenize(string input, Config &config)
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

bool Config::has_source_changed(Source source)
{
	if (!filesystem::exists(source.obj_path))
	{
		return true;
	}
	return filesystem::last_write_time(source.filepath) >
		   filesystem::last_write_time(source.obj_path);
}

Config::Config() : Config("ssbs") {}

Config::Config(std::string target_config)
{
	auto target = std::filesystem::current_path();
	target.append(target_config + ".cfg");
	if (!filesystem::exists(target))
	{
		ofstream writer;
		writer.open(target);
		if (!writer.is_open())
		{
			return;
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
		return;
	}
	if (!open_path(target.parent_path(), *this))
	{
		cerr << "Failed to load configurations from " << target << std::endl;
		return;
	}
	if (target.stem().string().compare(DEFAULT_TARGET_CONFIG) != 0)
	{
		load_config(target, *this);
	}
	parse_configs(*this);
}
