#include "include/ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

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

void ConfigParser::parse(int argc, char **argv)
{
    std::string config_path = (argc == 1 ? "configs/default.conf" : argv[1]);
    std::string content = this->readFile(config_path);

    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line))
    {
        line = removeComments(line);
        line = trim(line);

        if (!line.empty())
        {
            line = compactWhitespace(line);
            _lines.push_back(line);
        }
    }
}

std::string ConfigParser::removeComments(const std::string &line)
{
    size_t pos = line.find('#');
    if (pos != std::string::npos)
        return line.substr(0, pos);
    return line;
}

std::string ConfigParser::trim(const std::string &line)
{
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    size_t end = line.find_last_not_of(" \t\r\n");
    return line.substr(start, end - start + 1);
}

std::string ConfigParser::compactWhitespace(const std::string &line)
{
    std::string result;
    bool prev_space = false;

    for (size_t i = 0; i < line.length(); i++)
    {
        char c = line[i];
        if (c == ' ' || c == '\t')
        {
            if (!prev_space)
            {
                result += ' ';
                prev_space = true;
            }
        }
        else
        {
            result += c;
            prev_space = false;
        }
    }
    return result;
}