#include <iostream>
#include <filesystem>
#include <string>

#include "data.hpp"

int main(int argc, char *argv[])
{
	int a;

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
	if (resp.succeeded)
	{
		return 0;
	}
	printf("Failed to compile with %d errors, %d warnings\n", resp.errors.size(), resp.warnings.size());

	for (int i = 0; i < resp.errors.size(); i++)
	{
		LineError err = resp.errors[i];
		printf("Error %s : %d \n\t %s", err.file, err.line, err.message);
	}
	for (int i = 0; i < resp.warnings.size(); i++)
	{
		LineError err = resp.warnings[i];
		printf("Warning %s : %d \n\t %s", err.file, err.line, err.message);
	}

	return -1;
}