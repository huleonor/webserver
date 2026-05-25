#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <iostream>
#include <vector>

class ConfigParser
{
private:
	std::vector<std::string> _lines;
    std::string readFile(const std::string &filepath);
public:
	ConfigParser();
	~ConfigParser();

	void parse(int argc, char **argv);

private:
	std::string removeComments(const std::string &line);
	std::string trim(const std::string &line);
	std::string compactWhitespace(const std::string &line);

};

#endif