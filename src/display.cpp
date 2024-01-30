#include <iostream>
#include "display.hpp"
#include <filesystem>
// https://termcolor.readthedocs.io/
#include <termcolor.hpp>

using namespace std;

Display::Display(BuildResponse *response)
{
	this->response = response;
}

Display::~Display()
{
	// delete this->response;
}

string split_long_message(string msg)
{
	string buffer;
	int depth = 0;

	while (!msg.empty())
	{
		if (depth == 0)
		{
			depth++;
		}
		else
		{
			buffer += "\n\t  ";
		}
		if (msg.length() > PREFERRED_LINE_LENGTH)
		{
			int idx = msg.find_first_of(' ', PREFERRED_LINE_LENGTH);
			if (idx == -1)
			{
				buffer += msg;
				break;
			}
			buffer += msg.substr(0, idx);
			msg = msg.substr(idx);
		}
		else
		{
			buffer += msg;
			break;
		}
	}
	return buffer;
}

void Display::display_build_information()
{

	if (this->response->succeeded)
	{
		printf("Build completed with %d warnings\n", this->response->warnings);
	}
	else
	{
		printf("Failed to compile with %d errors, %d warnings\n", this->response->errors, this->response->warnings);
	}
	for (Source source : this->response->sources)
	{
		if (source.errors.size() == 0 && source.warnings.size() == 0)
		{
			continue;
		}
		filesystem::path relative_path = filesystem::relative(source.filepath);
		cout << termcolor::bold << "================================" << endl
			 << termcolor::reset;
		cerr << "In " << termcolor::underline << relative_path.string() << termcolor::reset << " : "
			 << "(" << termcolor::red << source.errors.size() << termcolor::reset << " errors, " << termcolor::yellow << source.warnings.size() << termcolor::reset << " warnings):" << endl;

		for (LineError err : source.errors)
		{
			string msg = split_long_message(err.message);
			cerr << termcolor::red << termcolor::bold << "  [" << relative_path.string() << ":" << err.line << ":" << err.column << "] Error: " << termcolor::reset << termcolor::red << msg << endl
				 << termcolor::reset;
		}
		for (LineError err : source.warnings)
		{
			string msg = split_long_message(err.message);
			cerr << termcolor::yellow << termcolor::bold << "  [" << relative_path.string() << ":" << err.line << ":" << err.column << "] Warning: " << termcolor::reset << termcolor::yellow << msg << endl
				 << termcolor::reset;
		}
		for (LineError err : source.notes)
		{
			string msg = split_long_message(err.message);
			cerr << termcolor::blue << termcolor::bold << "  [" << relative_path.string() << ":" << err.line << ":" << err.column << "] Note: " << termcolor::reset << termcolor::blue << msg << endl
				 << termcolor::reset;
		}
		if (!source.errors.empty() || !source.warnings.empty())
		{
			cout << endl
				 << "  See full logs at [" << source.log_path.string() << "]" << endl;
		}
		cout << termcolor::bold << "================================" << endl
			 << termcolor::reset;
	}
	if (this->response->warnings <= 0)
		return;
	if (this->response->succeeded)
	{
		printf("Build completed with %d warnings\n", this->response->warnings);
	}
	else
	{
		printf("Failed to compile with %d errors, %d warnings\n", this->response->errors, this->response->warnings);
	}
	cout << termcolor::bold << "================================" << endl
		 << termcolor::reset;
}