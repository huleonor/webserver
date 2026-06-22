#include "../../include/ConfigParser.hpp"

/* --------------------------------- Utils ---------------------------------- */

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

std::string ConfigParser::extractValue(const std::string &line)
{
    size_t pos = line.find(' ');
    if (pos == std::string::npos)
        throw std::runtime_error("Invalid directive: " + line);

    std::string value = line.substr(pos + 1);
    if (!value.empty() && value[value.length() - 1] == ';')
        value = value.substr(0, value.length() - 1);
    return value;
}
