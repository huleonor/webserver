#include "../../include/ConfigParser.hpp"

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}

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

void ConfigParser::parse(int argc, char **argv, ServerManager &manager)
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

    buildServers(manager);
}

void ConfigParser::buildServers(ServerManager &manager)
{
    for (size_t i = 0; i < _lines.size(); i++)
    {
        if (_lines[i] == "server {")
        {
            ServerConfig config;
            i++;
            parseServerBlock(i, config);
            manager.addServer(config);
        }
    }
}

void ConfigParser::parseServerBlock(size_t &i, ServerConfig &config)
{
    while (i < _lines.size() && _lines[i] != "}")
    {
        const std::string &line = _lines[i];

        if (line.substr(0, 6) == "listen")
            parseListenDirective(line, config);
        else if (line.substr(0, 4) == "host")
            parseHostDirective(line, config);
        else if (line.substr(0, 11) == "server_name")
            parseServerNameDirective(line, config);
        else if (line.substr(0, 4) == "root")
            parseRootDirective(line, config);
        else if (line.substr(0, 5) == "index")
            parseIndexDirective(line, config);
        else if (line.substr(0, 20) == "client_max_body_size")
            parseClientMaxBodySizeDirective(line, config);
        else if (line.substr(0, 10) == "error_page")
            parseErrorPageDirective(line, config);
        else if (line.substr(0, 8) == "location")
        {
            Location loc;
            size_t path_start = line.find(' ');
            size_t path_end = line.rfind(' ');
            if (path_start != std::string::npos && path_end != path_start)
                loc.setPath(line.substr(path_start + 1, path_end - path_start - 1));
            i++;
            parseLocationBlock(i, loc);
            config.addLocation(loc);
        }
        i++;
    }
}

void ConfigParser::parseLocationBlock(size_t &i, Location &location)
{
    while (i < _lines.size() && _lines[i] != "}")
    {
        const std::string &line = _lines[i];

        if (line.substr(0, 4) == "root")
            location.setRoot(extractValue(line));
        else if (line.substr(0, 5) == "index")
            location.setIndex(extractValue(line));
        else if (line.substr(0, 9) == "autoindex")
            location.setAutoindex(extractValue(line) == "on");
        else if (line.substr(0, 6) == "return")
            location.setReturn(extractValue(line));
        else if (line.substr(0, 13) == "allow_methods")
        {
            std::stringstream ss(extractValue(line));
            std::string method;
            while (ss >> method)
                location.addAllowMethod(method);
        }
        else if (line.substr(0, 8) == "cgi_path")
        {
            std::stringstream ss(extractValue(line));
            std::string path;
            while (ss >> path)
                location.addCgiPath(path);
        }
        else if (line.substr(0, 7) == "cgi_ext")
        {
            std::stringstream ss(extractValue(line));
            std::string ext;
            while (ss >> ext)
                location.addCgiExt(ext);
        }
        else if (line.substr(0, 11) == "upload_path")
            location.setUploadPath(extractValue(line));
        i++;
    }
}
