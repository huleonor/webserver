#include "include/ConfigParser.hpp"
#include <fstream>
#include <sstream>

ConfigParser::ConfigParser()
{
}

ConfigParser::~ConfigParser()
{
}

std::string ConfigParser::readFile(const std::string &filepath)
{
    std::ifstream file(filepath.c_str());

	if (!file.is_open())
	{
		std::cerr << "Error: Could not open file: " << filepath << std::endl;
		throw std::runtime_error("File not found");
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	return buffer.str();
}