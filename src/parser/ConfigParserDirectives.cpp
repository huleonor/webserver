#include "../../include/ConfigParser.hpp"

/* ------------------------------- Directives ------------------------------- */

void ConfigParser::parseListenDirective(const std::string &line, ServerConfig &config)
{
    std::string value = extractValue(line);

    std::stringstream ss(value);
    int port;
    ss >> port;

    if (ss.fail() || !ss.eof())
        throw std::runtime_error("Invalid port value: " + value);
    if (port < 1 || port > 65535)
        throw std::runtime_error("Port out of range (1-65535): " + value);

    config.setPort(static_cast<uint16_t>(port));
}

void ConfigParser::parseHostDirective(const std::string &line, ServerConfig &config)
{
    config.setHost(extractValue(line));
}

void ConfigParser::parseServerNameDirective(const std::string &line, ServerConfig &config)
{
    config.setServerName(extractValue(line));
}

void ConfigParser::parseRootDirective(const std::string &line, ServerConfig &config)
{
    config.setRoot(extractValue(line));
}

void ConfigParser::parseIndexDirective(const std::string &line, ServerConfig &config)
{
    config.setIndex(extractValue(line));
}

void ConfigParser::parseClientMaxBodySizeDirective(const std::string &line, ServerConfig &config)
{
    std::string value = extractValue(line);

    std::stringstream ss(value);
    unsigned long size;
    ss >> size;

    if (ss.fail() || !ss.eof())
        throw std::runtime_error("Invalid client_max_body_size: " + value);

    config.setClientMaxBodySize(size);
}

void ConfigParser::parseErrorPageDirective(const std::string &line, ServerConfig &config)
{
    size_t first = line.find(' ');
    if (first == std::string::npos)
        throw std::runtime_error("Invalid error_page directive: " + line);

    size_t second = line.find(' ', first + 1);
    if (second == std::string::npos)
        throw std::runtime_error("Invalid error_page directive: " + line);

    std::string code_str = line.substr(first + 1, second - first - 1);
    std::string page = line.substr(second + 1);

    if (!page.empty() && page[page.length() - 1] == ';')
        page = page.substr(0, page.length() - 1);

    std::stringstream ss(code_str);
    int code;
    ss >> code;

    if (ss.fail())
        throw std::runtime_error("Invalid error code: " + code_str);

    config.addErrorPage(code, page);
}
