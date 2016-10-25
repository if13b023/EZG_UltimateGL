#pragma once
#include <string>
#include <fstream>
#include <iostream>

class FileReader
{
public:
	std::string ReadText(const char* path);
	int ReadText(const char* path, char** output);
};
