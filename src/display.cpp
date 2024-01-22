#include <iostream>
#include "display.hpp"
#include <filesystem>
// https://termcolor.readthedocs.io/
#include <termcolor/termcolor.hpp>

using namespace std;
namespace fs = filesystem;
namespace col = termcolor;

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
		fs::path relative_path = fs::relative(source.filepath);
		cout << col::bold << "================================" << endl
			 << col::reset;
		cerr << "In " << col::underline << relative_path.string() << col::reset << " : "
			 << "(" << col::red << source.errors.size() << col::reset << " errors, " << col::yellow << source.warnings.size() << col::reset << " warnings):" << endl;

		for (LineError err : source.errors)
		{
			string msg = split_long_message(err.message);
			cerr << col::red << col::bold << "  [" << relative_path.string() << ":" << err.line << ":" << err.column << "] Error: " << col::reset << col::red << msg << endl
				 << col::reset;
		}
		for (LineError err : source.warnings)
		{
			string msg = split_long_message(err.message);
			cerr << col::yellow << col::bold << "  [" << relative_path.string() << ":" << err.line << ":" << err.column << "] Warning: " << col::reset << col::yellow << msg << endl
				 << col::reset;
		}
		cout << col::bold << "================================" << endl
			 << col::reset;
	}
}