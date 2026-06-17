#include "../../include/ServerManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h> 
#include <signal.h>
#include <poll.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <algorithm>

static std::string	toAddrStr(in_addr_t addr)
{
	struct in_addr tmp;
	tmp.s_addr = addr;
	return inet_ntoa(tmp);
}

/* ---------------------------- Member Attributes --------------------------- */
bool	ServerManager::_running = true;

/* ----------------------- Constructor and Destructor ----------------------- */
ServerManager::ServerManager() {}

ServerManager::~ServerManager()
{
	for (client_it it = _clients.begin(); it != _clients.end(); it++)
		delete it->second;
	for (size_t i = 0; i < _servers.size(); i++)
		close(_servers[i].getSocketFd());
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
			if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
				handleInitOrAcceptError(fd, "fcntl cloexec failed: " + std::string(strerror(errno)));
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
		catch(const std::exception& e)	{ std::cerr << "\033[31m[ERROR]: " << e.what() << "\033[0m\n"; }
	}
	if (_pfds.empty())
		throw std::runtime_error("no servers available, cannot continue");
}

/* --------------------------------- Runtime -------------------------------- */
bool	ServerManager::isServerSocket(size_t pos)	{ return (pos < _servers.size()); }

void	ServerManager::start()
{
	for (size_t i = 0; i < _servers.size(); i++)
		std::cout << "\033[33m[Server: " << _servers[i].getServerName() << ":" 
		<< _servers[i].getHost() << ":" << _servers[i].getPort() 
		<< "] Server running...\033[0m\n" << std::endl;
	while(_running)
	{
		int	num_events = poll(&_pfds[0], _pfds.size(), 60000);
		if (num_events == -1)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll failed "  + std::string(strerror(errno)));
		}
		if (num_events > 0)
			handleEvent();
		monitorClients();
	}
}

void	ServerManager::monitorClients()
{
	for (size_t i = _servers.size(); i < _pfds.size(); i++)
	{
		client_it	it = _clients.find(_pfds[i].fd);
		Client* c = it->second;
		if (c->getStatus() != Client::READING_HEADER && c->getStatus() != Client::READING_BODY)
            continue;
		if (time(NULL) - it->second->getLastTimeActivity() >= 60)
		{
			it->second->setLogMsg("timeout");
			it->second->buildErrorResponse(408);
			_pfds[i].events = POLLOUT;
		}
	}
}

void	ServerManager::handleEvent()
{
	for (size_t i = 0; i < _pfds.size(); i++)
	{
		if (_pfds[i].revents & POLLERR)
		{
			client_it	it = _clients.find(_pfds[i].fd);
			if (it != _clients.end())
			{
				it->second->setLogMsg("Connection error (POLLERR)");
				closeConnection(i); continue;
			}
		}
		if (_pfds[i].revents & POLLIN)
		{
			if (isServerSocket(i))
				acceptNewClient(i);
			else if (_clients.count(_pfds[i].fd))
				handleClientRequest(i);
		}
		if (_pfds[i].revents & POLLOUT)
			handleClientResponse(i);
		if (_pfds[i].revents & POLLHUP)
		{
			if (_clients.count(_pfds[i].fd))
				handlePollHup(i); 
		}
	}
}

void	ServerManager::handlePollHup(size_t& pfds_pos)
{
	if (_clients.count(_pfds[pfds_pos].fd))
	{
		Client*	client = _clients[_pfds[pfds_pos].fd];
		client->setLogMsg("Connection close (POLLHUP)");
		closeConnection(pfds_pos);
	}
}

/* ---------------------------- Client Management --------------------------- */
void	ServerManager::acceptNewClient(size_t pfds_pos)
{
	while (true)
	{
		struct	sockaddr_in	addr = {};
		socklen_t	addr_size = sizeof(addr);
	
		try
		{
			int	client_fd = accept(_servers[pfds_pos].getSocketFd(), (struct sockaddr *)&addr, &addr_size);
			if (client_fd == -1)
				break;
			if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
				handleInitOrAcceptError(client_fd, "fcntl failed: " + std::string(strerror(errno)));
			if (fcntl(client_fd, F_SETFD, FD_CLOEXEC) == -1)
				handleInitOrAcceptError(client_fd, "fcntl cloexec failed: " + std::string(strerror(errno)));
			_clients.insert(std::make_pair(client_fd, new Client(client_fd, addr, _servers[pfds_pos])));
			struct pollfd pfd = {};
			pfd.fd = client_fd;
			pfd.events = POLLIN;
			_pfds.push_back(pfd);
			std::cout << "\033[32m[INFO]: " << toAddrStr(addr.sin_addr.s_addr) << ":"
					  << ntohs(addr.sin_port) << " | connected\033[0m" << std::endl;
		}
		catch(const std::exception& e) { std::cerr << "\033[31m[ERROR]: " << e.what() << "\033[0m\n"; }
	}
}

void	ServerManager::handleClientRequest(size_t& pfds_pos)
{
	client_it	it = _clients.find(_pfds[pfds_pos].fd);
	it->second->setLastTimeActivity(time(NULL));
	try
	{
		ssize_t	n = it->second->receiveData();
		if (n <= 0)
		{
			std::string	msg = (n == -1) ?  "recv failed: " + std::string(strerror(errno)) : "";
			it->second->setLogMsg(msg);
			return closeConnection(pfds_pos);
		}
		if (it->second->getStatus() == Client::PROCESSING)
			processClientRequest(*it->second);
		if (it->second->getStatus() == Client::WRITING || it->second->getStatus() == Client::ERROR)
			_pfds[pfds_pos].events = POLLOUT;
	}
	catch(const std::exception& e)
	{
		std::cerr << "[ERROR] " << e.what() << std::endl;
		it->second->setLogMsg(e.what());
		it->second->buildErrorResponse(500);
		_pfds[pfds_pos].events = POLLOUT;
	}
}

void	ServerManager::handleClientResponse(size_t& pfds_pos)
{
	client_it	it = _clients.find(_pfds[pfds_pos].fd);
	const HttpRequest&	request = it->second->getRequest();
	const Response&	response = it->second->getResponse();
	std::string	addr = toAddrStr(it->second->getClientAddr());
	int			port = ntohs(it->second->getClientPort());

	it->second->sendResponse();
	if (it->second->getStatus() == Client::ERROR)
	{
		it->second->setLogMsg("send failed: " + std::string(strerror(errno)));
		it->second->setStatus(Client::CLOSE);
	}
	std::cout << "[RESPONSE]: " << addr << ":" << port << " | Request:"
			<< request.line_request << " -> " << response.getFirstLine();
	if (it->second->getStatus() == Client::CLOSE)
		closeConnection(pfds_pos);
}

void	ServerManager::processClientRequest(Client& client)
{
	const Location*	location = client.getClientServer()->findLocation(client.getRequest().path);
	if (location == NULL)
		return client.buildErrorResponse(404);
	if (location->isValidMethod(client.getRequest().method) == false)
		return client.buildErrorResponse(405);
	client.validateAndReplacePath(*location);
	if (client.getStatus() == Client::ERROR)
		return ;
	if (!location->getCgiExt().empty() && !location->getCgiPath().empty())
	{
		client.handleCGI(*location);
		if (client.getStatus() == Client::ERROR)
			return ;
		struct pollfd pfd = client.getCgi()->cgiSetup();
		_pfds.push_back(pfd);
		return ;
	}
	if (client.getRequest().method == "GET")
		client.handleGet(*location);
	else if (client.getRequest().method == "POST")
		client.handlePost(*location);
	else if (client.getRequest().method == "DELETE")
		client.handleDelete();
}

void	ServerManager::closeConnection(size_t& pfds_pos)
{
	client_it	it = _clients.find(_pfds[pfds_pos].fd);
	std::string	addr = toAddrStr(it->second->getClientAddr());
	int			port = ntohs(it->second->getClientPort());
	const std::string	msg = it->second->getLogMsg().empty() ? "closed by client" : it->second->getLogMsg();
	delete it->second;
	_clients.erase(_pfds[pfds_pos].fd);
	_pfds.erase(_pfds.begin() + pfds_pos);
	pfds_pos--;
	std::cout << "\033[31m[INFO]: " <<  addr << ":" 
			  << port  << " | disconnected (" << msg << ")\033[0m" 
			  << std::endl;
}

/* ----------------------------- Signal handling ---------------------------- */
void	ServerManager::handleSignal(int sig)
{
	(void)sig;
	const char msg[] = "\033[31m\n[INFO]: Server stopped by signal\033[0m\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	_running = false;
}
