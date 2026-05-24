#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <iostream>

class ConfigParser
{
private:

public:
	ConfigParser();
	~ConfigParser();

	std::string readFile(const std::string &filepath);
};

#endif