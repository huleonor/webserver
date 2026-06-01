#include "../../include/ServerManager.hpp"
#include <sys/socket.h>   
#include <netinet/in.h>   
#include <arpa/inet.h>  
#include <unistd.h>   
#include <iostream>
#include <fcntl.h> 
#include <poll.h>     
#include <cstring>      
#include <cerrno>        
#include <stdexcept> 
#include <algorithm>

/* ---------------------------- Static Members --------------------------- */
bool	ServerManager::_running = true;

void	ServerManager::handleSignal(int sig)
{
	(void)sig;
	_running = false;
}

/* ----------------------- Constructor and Destructor ----------------------- */
ServerManager::ServerManager() {}
ServerManager::~ServerManager() {}

/* -------------------------- ServerManager Methods ------------------------- */
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

/*
For each server: create a non-blocking socket, bind it to the configured host:port,
and start listening. Each socket is then registered in _pfds so poll() can detect incoming connections.
*/
void	ServerManager::setupServers()
{
	for (size_t i = 0; i < _servers.size(); i++)
	{
		try
		{
			int	fd = socket(AF_INET, SOCK_STREAM, 0);
			if (fd == -1)
				throw std::runtime_error("socket failed: " +  std::string(strerror(errno)));
			if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
				handleInitOrAcceptError(fd, "fcntl failed: " + std::string(strerror(errno)));
			int	optval = 1;
			if (setsockopt(fd,  SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
				handleInitOrAcceptError(fd, "setsockopt failed: " +  std::string(strerror(errno)));
			struct	sockaddr_in addr = {};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(_servers[i].getPort());
    		in_addr_t ip = inet_addr(_servers[i].getHost().c_str());
			if (ip == INADDR_NONE)
				handleInitOrAcceptError(fd, "invalid host: " + _servers[i].getHost());
			addr.sin_addr.s_addr = ip;
			if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
				handleInitOrAcceptError(fd, "bind failed: " +  std::string(strerror(errno)));
			if (listen(fd, 128) == -1)
				handleInitOrAcceptError(fd, "listen failed: " +  std::string(strerror(errno)));
			_servers[i].setSocketFd(fd);
			struct pollfd pfd = {};
			pfd.fd = fd;
			pfd.events = POLLIN;
			_pfds.push_back(pfd);
		}
		catch(const std::exception& e) 
		{ 
			std::cerr << "\033[31mError: " << e.what() << "\033[0m\n"; 
			if (_servers.size() == 1)
				throw std::runtime_error("no servers available, cannot continue");
		}
	}
}

void	ServerManager::start()
{
	while(_running)
	{
		try
		{
			int	num_events = poll(&_pfds[0], _pfds.size(), 5000);

			if (num_events > 0)
				handleEvent();
		}
		catch(const std::exception& e) { std::cerr << "\033[31mError: " << e.what() << "\033[0m\n"; };
	}
}

/* ----------------------------- Private Methods ---------------------------- */
void	ServerManager::handleEvent()
{
	for (size_t i = 0; i < _pfds.size(); i++)
	{
		if (i < _servers.size())
		{
			if (_pfds[i].revents & POLLIN)
				acceptNewClient(i);
		}
		else
		{
			if (_pfds[i].revents == POLLIN)
				handleClientRequest(i);
		}
	}
}

void	ServerManager::handleClientRequest(int pfds_pos)
{
	char	buffer[4096] = {0};
	ServerManager::client_it	it = _clients.find(_pfds[pfds_pos].fd);

	ssize_t	n = recv(_pfds[pfds_pos].fd, &buffer, sizeof(buffer), 0);
	if (n <= 0)
		handleClientError(pfds_pos, "recv: impossible to read data");
	if (it->second.getStatus() == Client::READING_HEADER)
		it->second.receiveHeader(buffer);
	else
		it->second.receiveBody(buffer);
}

/* --------------------------------- Methods -------------------------------- */
void	ServerManager::acceptNewClient(int pfds_pos)
{
	struct	sockaddr_in	addr = {};
	socklen_t	addr_size = sizeof(addr);

	int	client_fd = accept(_servers[pfds_pos].getSocketFd(), (struct sockaddr *)&addr, &addr_size);
	if (client_fd == -1)
		handleInitOrAcceptError(client_fd, "fcntl client failed: " + std::string(strerror(errno)));
	Client	newClient(_servers[pfds_pos], client_fd, addr);
	_clients.insert(std::make_pair(client_fd, newClient));
	struct pollfd pfd = {};
	pfd.fd = client_fd;
	pfd.events = POLLIN;
	_pfds.push_back(pfd);
}

/* ---------------------------- Helper functions ---------------------------- */
void	ServerManager::handleInitOrAcceptError(int fd, const std::string& msg)
{
	if (fd > 0)
		close(fd);
	throw std::runtime_error(msg);
}

void	ServerManager::handleClientError(int pfds_pos, const std::string& msg)
{
	_pfds.erase(_pfds.begin() + pfds_pos);
	_clients.erase(_pfds[pfds_pos].fd);
	close(_pfds[pfds_pos].fd);
	throw std::runtime_error(msg);
}