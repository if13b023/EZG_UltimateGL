#include "FileIO.h"

std::string FileReader::ReadText(const char* path)
{
	std::ifstream file(path);

	if (!file.is_open())
	{
		//std::cout << "Failed to open file: " << path << std::endl;
		return "Error";
	}

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	return content;
}

int FileReader::ReadText(const char * path, char ** output)
{
	std::string tmp = ReadText(path);

	*output = new char[tmp.length() + 1];
	memcpy(*output, tmp.c_str(), tmp.length()+1);

	return tmp.length();
}


