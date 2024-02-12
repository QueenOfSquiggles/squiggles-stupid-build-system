#include <iostream>
#include <filesystem>
#include <string>

#include "build.hpp"
#include "display.hpp"
#include "cmake.hpp"

struct CommandArgs
{
	std::string configuration;
	std::string command;
	std::vector<std::string> flags;
};

CommandArgs parse_command(int argc, char *argv[])
{
	CommandArgs cmd;
	std::vector<std::string> args;
	if (argc >= 2)
	{
		for (int i = 1; i < argc; i++)
		{
			auto arg = std::string(argv[i]);
			if (arg.substr(0, 2).compare("--") == 0)
			{
				// flags
				cmd.flags.push_back(arg.substr(2));
			}
			else
			{
				args.push_back(arg);
			}
		}
	}
	switch (args.size())
	{
	case 0:
		cmd.configuration = DEFAULT_TARGET_CONFIG;
		cmd.command = "build";
		break;
	case 1:
		cmd.configuration = DEFAULT_TARGET_CONFIG;
		cmd.command = args[0];
		break;
	case 2:

		cmd.configuration = args[0].compare(".") == 0 ? DEFAULT_TARGET_CONFIG : args[0];
		cmd.command = args[1];
		break;
	default:
		std::cerr << "Error: too many commands found!" << std::endl;
		break;
	}
	return cmd;
}

int main(int argc, char *argv[])
{

	auto cmd = parse_command(argc, argv);
	if (cmd.command.compare("build") == 0)
	{
		std::filesystem::path pwd = std::filesystem::current_path();
		StupidBuild builder;
		BuildResponse resp = builder.build_target(cmd.configuration);
		Display disp(&resp);
		disp.display_build_information();
		return resp.succeeded ? 0 : -1;
	}
	else if (cmd.command.compare("cmake") == 0)
	{
		bool resp = CMakeGenerator::generate_cmake_config(cmd.configuration, cmd.flags);
		auto msg = std::string(resp ? "Successfully created CMake configuration" : "Oops! Something went wrong creating CMake configs.");
		std::cout << msg << std::endl;
		return resp ? 1 : 0;
	}
	return 0;
}