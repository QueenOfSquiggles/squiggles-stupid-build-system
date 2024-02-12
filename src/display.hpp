#pragma once
#include "build.hpp"

#define PREFERRED_LINE_LENGTH 75

class Display
{
	BuildResponse *response;

public:
	Display(BuildResponse *response);
	~Display();
	void display_build_information();
};