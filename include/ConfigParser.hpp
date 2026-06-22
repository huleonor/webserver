#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ServerConfig.hpp"
#include "ServerManager.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

class ConfigParser
{
private:
// --- Attributes ---
	std::vector<std::string>	_lines;

// --- Non-copyable ---
	ConfigParser(const ConfigParser& copy);
	ConfigParser&	operator=(const ConfigParser& copy);

// --- Internal ---
	std::string	readFile(const std::string& filepath);

public:
// --- Lifecycle ---
	ConfigParser();
	~ConfigParser();

// --- Utils ---
	std::string	removeComments(const std::string& line);
	std::string	trim(const std::string& line);
	std::string	compactWhitespace(const std::string& line);
	std::string	extractValue(const std::string& line);

// --- Parsing ---
	void		parse(int argc, char** argv, ServerManager& manager);
	void		buildServers(ServerManager& manager);
	void		parseServerBlock(size_t& i, ServerConfig& config);
	void		parseLocationBlock(size_t& i, Location& location);

// --- Directives ---
	void		parseListenDirective(const std::string& line, ServerConfig& config);
	void		parseHostDirective(const std::string& line, ServerConfig& config);
	void		parseServerNameDirective(const std::string& line, ServerConfig& config);
	void		parseRootDirective(const std::string& line, ServerConfig& config);
	void		parseIndexDirective(const std::string& line, ServerConfig& config);
	void		parseClientMaxBodySizeDirective(const std::string& line, ServerConfig& config);
	void		parseErrorPageDirective(const std::string& line, ServerConfig& config);
};

#endif
