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

/* ---------------------------- Member Attributes --------------------------- */
bool	ServerManager::_running = true;

/* ----------------------- Constructor and Destructor ----------------------- */
ServerManager::ServerManager() {}

ServerManager::~ServerManager()
{
	for (client_it it = _clients.begin(); it != _clients.end(); it++)
		delete it->second;
}

/* ----------------------------- Error handling ----------------------------- */
void	ServerManager::handleInitOrAcceptError(int fd, const std::string& msg)
{
	if (fd != -1)
		close(fd);
	throw std::runtime_error(msg);
}

/* --------------------------------- Getters -------------------------------- */
std::vector<ServerConfig> &ServerManager::getServers()	{ return _servers; }
size_t ServerManager::size()                        	{ return _servers.size(); }

/* ---------------------------------- Setup --------------------------------- */
void ServerManager::addServer(const ServerConfig &server) { _servers.push_back(server); }

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
		catch(const std::exception& e)	{ std::cerr << "\033[31mError: " << e.what() << "\033[0m\n"; }
	}
	if (_pfds.empty())
		throw std::runtime_error("no servers available, cannot continue");
}

/* --------------------------------- Runtime -------------------------------- */
bool	ServerManager::isServerSocket(size_t pos)	{ return (pos < _servers.size()); }

void	ServerManager::start()
{
	while(_running)
	{
		int	num_events = poll(&_pfds[0], _pfds.size(), 5000);
		// if (num_events == -1)
		if (num_events > 0)
			handleEvent();
	}
}

void	ServerManager::handleEvent()
{
	for (size_t i = 0; i < _pfds.size(); i++)
	{
		try
		{
			if (_pfds[i].revents & POLLIN)
			{
				if (isServerSocket(i))
					acceptNewClient(i);
				else
					handleClientRequest(i);
			}
			else if (_pfds[i].revents & POLLOUT)
				handleClientResponse(i);
		}
		catch(const std::exception& e) 
		{ 
			std::cerr << "\033[31mError: " << e.what() << "\033[0m\n";
		};
	}
}

/* ---------------------------- Client Management --------------------------- */
void	ServerManager::acceptNewClient(size_t pfds_pos)
{
	while (true)
	{
		struct	sockaddr_in	addr = {};
		socklen_t	addr_size = sizeof(addr);
	
		int	client_fd = accept(_servers[pfds_pos].getSocketFd(), (struct sockaddr *)&addr, &addr_size);
		if (client_fd == -1)
			break;
		if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
			handleInitOrAcceptError(client_fd, "fcntl failed: " + std::string(strerror(errno)));
		_clients.insert(std::make_pair(client_fd, new Client(client_fd, addr, _servers[pfds_pos])));
		struct pollfd pfd = {};
		pfd.fd = client_fd;
		pfd.events = POLLIN;
		_pfds.push_back(pfd);
		std::cout << "\033[32m[INFO]: " 
				  << inet_ntoa(addr.sin_addr) << ":" 
				  << ntohs(addr.sin_port) 
				  << " | connected\033[0m" 
				  << std::endl;
	}
}

void	ServerManager::handleClientRequest(size_t& pfds_pos)
{
	ServerManager::client_it	it = _clients.find(_pfds[pfds_pos].fd);
	ssize_t	n = it->second->receiveData();

	if (n == -1)
		closeConnection(pfds_pos, "recv failed: " + std::string(strerror(errno)));
	else if (n == 0)
		closeConnection(pfds_pos, "");
	else if (it->second->getStatus() == Client::ERROR)
		_pfds[pfds_pos].events = POLLOUT;
}

void	ServerManager::handleClientResponse(size_t& pfds_pos)
{
	ServerManager::client_it	it = _clients.find(_pfds[pfds_pos].fd);
	it->second->sendResponse();
	std::string	msg = "";
	if (it->second->getStatus() == Client::ERROR)
	{
		msg = "send failed: " + std::string(strerror(errno));
		it->second->setStatus(Client::CLOSE);
	}
	if (it->second->getStatus() == Client::CLOSE)
		closeConnection(pfds_pos, msg);
}

void	ServerManager::closeConnection(size_t& pfds_pos, const std::string& msg)
{
	struct	in_addr	tmp;
	ServerManager::client_it	it = _clients.find(_pfds[pfds_pos].fd);

	tmp.s_addr = it->second->getClientAddr();
	std::string	addr = inet_ntoa(tmp);
	int	port = ntohs(it->second->getClientPort());
	delete it->second;
	_clients.erase(_pfds[pfds_pos].fd);
	_pfds.erase(_pfds.begin() + pfds_pos);
	pfds_pos--;
	std::cout << "\033[31m[INFO]: " <<  addr << ":" 
			  << port  << " | disconnected\033[0m" 
			  << std::endl;
	if (!msg.empty())
		throw std::runtime_error(msg);
}

/* ----------------------------- Signal handling ---------------------------- */
void	ServerManager::handleSignal(int sig)
{
	(void)sig;
	_running = false;
}

/* ----------------------------------- Tmp ---------------------------------- */
void ServerManager::print()
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
