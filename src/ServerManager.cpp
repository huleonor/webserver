#include "include/ServerManager.hpp"
#include <iostream>

ServerManager::ServerManager() {}
ServerManager::~ServerManager() {}

void ServerManager::addServer(const ServerConfig &server) { _servers.push_back(server); }
std::vector<ServerConfig> &ServerManager::getServers()    { return _servers; }
size_t ServerManager::size() const                        { return _servers.size(); }

void ServerManager::print() const
{
	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::cout << "=== Server " << i + 1 << " ===" << std::endl;
		std::cout << "  host:              " << _servers[i].getHost() << std::endl;
		std::cout << "  port:              " << _servers[i].getPort() << std::endl;
		std::cout << "  server_name:       " << _servers[i].getServerName() << std::endl;
		std::cout << "  root:              " << _servers[i].getRoot() << std::endl;
		std::cout << "  index:             " << _servers[i].getIndex() << std::endl;
		std::cout << "  max_body_size:     " << _servers[i].getClientMaxBodySize() << std::endl;

		std::map<int, std::string> error_pages = _servers[i].getErrorPages();
		for (std::map<int, std::string>::iterator it = error_pages.begin(); it != error_pages.end(); it++)
			std::cout << "  error_page:        " << it->first << " -> " << it->second << std::endl;

		std::vector<Location> locations = _servers[i].getLocations();
		for (size_t j = 0; j < locations.size(); j++)
		{
			std::cout << "  location " << locations[j].getPath() << ":" << std::endl;
			if (!locations[j].getRoot().empty())
				std::cout << "    root:            " << locations[j].getRoot() << std::endl;
			if (!locations[j].getIndex().empty())
				std::cout << "    index:           " << locations[j].getIndex() << std::endl;
			std::cout << "    autoindex:       " << (locations[j].getAutoindex() ? "on" : "off") << std::endl;
			if (!locations[j].getReturn().empty())
				std::cout << "    return:          " << locations[j].getReturn() << std::endl;
			if (!locations[j].getUploadPath().empty())
				std::cout << "    upload_path:     " << locations[j].getUploadPath() << std::endl;

			std::vector<std::string> methods = locations[j].getAllowMethods();
			if (!methods.empty())
			{
				std::cout << "    allow_methods:   ";
				for (size_t k = 0; k < methods.size(); k++)
					std::cout << methods[k] << (k + 1 < methods.size() ? " " : "");
				std::cout << std::endl;
			}

			std::vector<std::string> cgi_ext = locations[j].getCgiExt();
			if (!cgi_ext.empty())
			{
				std::cout << "    cgi_ext:         ";
				for (size_t k = 0; k < cgi_ext.size(); k++)
					std::cout << cgi_ext[k] << (k + 1 < cgi_ext.size() ? " " : "");
				std::cout << std::endl;
			}

			std::vector<std::string> cgi_path = locations[j].getCgiPath();
			if (!cgi_path.empty())
			{
				std::cout << "    cgi_path:        ";
				for (size_t k = 0; k < cgi_path.size(); k++)
					std::cout << cgi_path[k] << (k + 1 < cgi_path.size() ? " " : "");
				std::cout << std::endl;
			}
		}
	}
}
