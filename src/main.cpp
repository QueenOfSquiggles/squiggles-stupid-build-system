#include <iostream>
#include <filesystem>
#include <string>

#include "data.hpp"
#include "display.hpp"

int main(int argc, char *argv[])
{
	std::filesystem::path pwd = std::filesystem::current_path();
	StupidBuild builder;
	if (!builder
			 .open_path(pwd.generic_string()))
	{
		std::cerr << "Failed to open current directory. [" << pwd.generic_string() << "]" << std::endl;
		return -1;
	}

	std::string target(DEFAULT_TARGET_CONFIG);
	if (argc >= 2)
	{
		target = std::string(argv[1]);
	}

	BuildResponse resp = builder.build_target(target);
	Display disp(&resp);
	disp.display_build_information();
	return resp.succeeded ? 0 : -1;
}