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
#include <sys/wait.h>

/* --------------------------------- Static --------------------------------- */
static std::string	toAddrStr(in_addr_t addr)
{
	struct in_addr tmp;
	tmp.s_addr = addr;
	return inet_ntoa(tmp);
}

/* ---------------------------- Internal: Client ---------------------------- */
void	ServerManager::acceptNewClient(size_t pfds_pos)
{
	while (true)
	{
		struct sockaddr_in	addr = {};
		socklen_t	addr_size = sizeof(addr);

		try
		{
			int	client_fd = accept(_servers[pfds_pos].getSocketFd(), (struct sockaddr*)&addr, &addr_size);
			if (client_fd == -1)
				break;
			if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
				handleInitOrAcceptError(client_fd, "fcntl failed: " + std::string(strerror(errno)));
			_clients.insert(std::make_pair(client_fd, new Client(client_fd, addr, _servers[pfds_pos])));
			struct pollfd pfd = {};
			pfd.fd = client_fd;
			pfd.events = POLLIN;
			_pfds.push_back(pfd);
			std::cout << "\033[32m[INFO]: " << toAddrStr(addr.sin_addr.s_addr) << ":"
					  << ntohs(addr.sin_port) << " | connected on: " 
					  << _servers[pfds_pos].getPort() << "\033[0m" << std::endl;
		}
		catch (const std::exception& e) { std::cerr << "\033[31m[ERROR]: " << e.what() << "\033[0m\n"; }
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
			std::string	msg = (n == -1) ? "recv failed" : "";
			it->second->setLogMsg(msg);
			return closeConnection(pfds_pos);
		}
		if (it->second->getStatus() == Client::PROCESSING)
			processClientRequest(*it->second);
		if (it->second->getStatus() == Client::WRITING || it->second->getStatus() == Client::ERROR)
			_pfds[pfds_pos].events = POLLOUT;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[ERROR] " << e.what() << std::endl;
		it->second->setLogMsg(e.what());
		it->second->buildErrorResponse(500);
		_pfds[pfds_pos].events = POLLOUT;
	}
}

void	ServerManager::handleClientResponse(size_t& pfds_pos)
{
	client_it		it = _clients.find(_pfds[pfds_pos].fd);
	const HttpRequest&	request = it->second->getRequest();
	const Response&		response = it->second->getResponse();
	std::string	addr = toAddrStr(it->second->getClientAddr());
	int			port = ntohs(it->second->getClientPort());

	it->second->sendResponse();
	if (it->second->getStatus() == Client::ERROR)
	{
		it->second->setLogMsg("send failed");
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
	if (!location->getCgiExt().empty() && !location->getCgiPath().empty()
			&& location->findFileExtension(client.getRequest().path))
	{
		client.handleCGI(*location);
		if (client.getStatus() == Client::ERROR)
			return ;
		struct pollfd pfd = client.getCgi()->cgiSetup();
		_pfds.push_back(pfd);
		_cgi_pipes[pfd.fd] = &client;
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
	client_it		it = _clients.find(_pfds[pfds_pos].fd);
	std::string		addr = toAddrStr(it->second->getClientAddr());
	int				port = ntohs(it->second->getClientPort());
	const std::string	msg = it->second->getLogMsg().empty() ? "closed by client" : it->second->getLogMsg();
	CgiHandler*		cgi = it->second->getCgi();
	if (cgi && cgi->getPid() > 0)
	{
		kill(cgi->getPid(), SIGKILL);
		cgi->checkWaitpid();
		for (size_t j = 0; j < _pfds.size(); j++)
		{
			if (_cgi_pipes.count(_pfds[j].fd) && _cgi_pipes[_pfds[j].fd] == it->second)
			{
				cgi->closeAllOpenPipeFds();
				_cgi_pipes.erase(_pfds[j].fd);
				_pfds.erase(_pfds.begin() + j);
				if (j < pfds_pos) pfds_pos--;
				break;
			}
		}
	}
	delete it->second;
	_clients.erase(_pfds[pfds_pos].fd);
	_pfds.erase(_pfds.begin() + pfds_pos);
	pfds_pos--;
	std::cout << "\033[31m[INFO]: " << addr << ":"
			  << port << " | disconnected (" << msg << ")\033[0m"
			  << std::endl;
}

void	ServerManager::handleClientPollCleanUp(size_t& pfds_pos)
{
	Client*	client = _clients[_pfds[pfds_pos].fd];
	client->setLogMsg("Connection closed (POLLHUP or POLLERR)");
	closeConnection(pfds_pos);
}

bool	ServerManager::hasCgiClients()	{ return (!_cgi_pipes.empty()); }

/* ----------------------------- Internal: CGI ------------------------------ */
void	ServerManager::handleCgiProcess(size_t& pfds_pos)
{
	Client*		client = _cgi_pipes[_pfds[pfds_pos].fd];
	CgiHandler*	cgi = client->getCgi();
	char		buffer[4096] = {0};

	ssize_t	n = read(_pfds[pfds_pos].fd, buffer, sizeof(buffer));
	if (n < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;
		handleCgiPollCleanUp(pfds_pos, "CGI pipe closed without process exit"); return ;
	}
	if (n > 0)
		{ cgi->receiveCgiOutput(std::string(buffer, n)); return ; }
	if (!cgi->getCgiOutputBuffer().empty() || cgi->checkWaitpid() == 0)
		{ processCgiOutput(client, cgi, pfds_pos); return; }
	closeFdAndCleanMaps(cgi, pfds_pos, true);
	cgi->setStatus(CgiHandler::ERROR);
	processCgiClientResponse(client, 500, "");
	client->setLogMsg("CGI pipe closed without process exit");
}

void	ServerManager::sendCgiBody(size_t& pfds_pos)
{
	Client*		client = _cgi_pipes[_pfds[pfds_pos].fd];
	CgiHandler*	cgi = client->getCgi();

	cgi->sendBody(client->getRequest().body);
	if (cgi->getStatus() == CgiHandler::DONE)
	{
		cgi->closePipeFd(cgi->getPipeBody()[1]);
		closeFdAndCleanMaps(cgi, pfds_pos, false);
		struct pollfd	pfd = {};
		pfd.fd = cgi->getPipeOutput()[0];
		pfd.events = POLLIN;
		_pfds.push_back(pfd);
		_cgi_pipes[pfd.fd] = client;
		cgi->setStatus(CgiHandler::NONE);
	}
	if (cgi->getStatus() == CgiHandler::ERROR)
	{
		cgi->checkWaitpid();
		closeFdAndCleanMaps(cgi, pfds_pos, true);
		processCgiClientResponse(client, 500, "");
		client->setLogMsg("CGI pipe closed without process exit");
	}
}

void	ServerManager::processCgiOutput(Client* client, CgiHandler* cgi, size_t& pfds_pos)
{
	const std::string&	output = cgi->getCgiOutputBuffer();
	size_t				sep = output.find("\r\n\r\n");
	size_t				offset = 4;
	std::string			body;
	std::string			content_type = "text/html";
	int					status_code = 200;

	if (sep == std::string::npos)
	{
		sep = output.find("\n\n");
		offset = 2;
	}
	if (sep == std::string::npos)
		body = output;
	else
	{
		std::string	cgi_headers = output.substr(0, sep);
		size_t ct = cgi_headers.find("Content-Type:");
		if (ct != std::string::npos)
		{
			size_t end = cgi_headers.find("\n", ct);
			content_type = cgi_headers.substr(ct + 13, end - ct - 13);
			size_t s = content_type.find_first_not_of(" \r");
			if (s != std::string::npos) content_type = content_type.substr(s);
			size_t e = content_type.find_last_not_of(" \r");
			if (e != std::string::npos) content_type = content_type.substr(0, e + 1);
		}
		size_t st = cgi_headers.find("Status:");
		if (st != std::string::npos)
		{
			size_t end = cgi_headers.find("\n", st);
			std::string status_str = cgi_headers.substr(st + 7, end - st - 7);
			size_t s = status_str.find_first_not_of(" \r");
			if (s != std::string::npos)
				status_code = std::atoi(status_str.substr(s).c_str());
		}
		body = output.substr(sep + offset);
	}
	closeFdAndCleanMaps(cgi, pfds_pos, true);
	processCgiClientResponse(client, status_code, body, content_type);
}

void	ServerManager::closeFdAndCleanMaps(CgiHandler* cgi, size_t& pfds_pos, bool closeFds)
{
	if (closeFds == true)
		cgi->closeAllOpenPipeFds();
	_cgi_pipes.erase(_pfds[pfds_pos].fd);
	_pfds.erase(_pfds.begin() + pfds_pos);
	pfds_pos--;
}

void	ServerManager::processCgiClientResponse(Client* client, int code, const std::string body, const std::string content_type)
{
	int	client_socket = client->getClientSocket();
	if (client->getCgi()->getStatus() == CgiHandler::ERROR)
		client->buildErrorResponse(code);
	else
		client->buildCgiResponse(body, content_type, code);
	for (size_t i = 0; i < _pfds.size(); i++)
	{
		if (_pfds[i].fd == client_socket)
		{
			_pfds[i].events = POLLOUT;
			break;
		}
	}
}

void	ServerManager::handleCgiPollCleanUp(size_t& pfds_pos, const std::string& logMsg)
{
	Client*		client = _cgi_pipes[_pfds[pfds_pos].fd];
	CgiHandler*	cgi = client->getCgi();

	cgi->checkWaitpid();
	closeFdAndCleanMaps(cgi, pfds_pos, true);
	cgi->setStatus(CgiHandler::ERROR);
	processCgiClientResponse(client, 500, "");
	client->setLogMsg(logMsg);
}

/* --------------------------- Internal: Runtime ---------------------------- */
void	ServerManager::handleEvent()
{
	for (size_t i = 0; i < _pfds.size(); i++)
	{
		if ((_pfds[i].events & POLLIN) && (_pfds[i].revents & (POLLIN | POLLHUP | POLLERR)))
		{
			if (isServerSocket(i))
				acceptNewClient(i);
			else if (_clients.count(_pfds[i].fd))
				handleClientRequest(i);
			else
				handleCgiProcess(i);
			continue;
		}
		if ((_pfds[i].events & POLLOUT) && (_pfds[i].revents & (POLLOUT | POLLHUP | POLLERR)))
		{
			if (_clients.count(_pfds[i].fd))
				_pfds[i].revents & (POLLHUP | POLLERR) ? handleClientPollCleanUp(i) : handleClientResponse(i);
			else
				_pfds[i].revents & (POLLHUP | POLLERR)
					? handleCgiPollCleanUp(i, "CGI closed (POLLHUP or POLLERR)") : sendCgiBody(i);
		}
	}
}

void	ServerManager::monitorClients()
{
	for (size_t i = _servers.size(); i < _pfds.size(); i++)
	{
		client_it	it = _clients.find(_pfds[i].fd);
		if (it == _clients.end())
			continue;
		Client* c = it->second;
		if ((c->getStatus() == Client::READING_HEADER || c->getStatus() == Client::READING_BODY)
			&& time(NULL) - c->getLastTimeActivity() >= 60)
		{
			c->setLogMsg("timeout");
			c->buildErrorResponse(408);
			_pfds[i].events = POLLOUT;
		}
		CgiHandler* cgi = c->getCgi();
		if (cgi && cgi->getPid() > 0 && time(NULL) - cgi->getStartTime() >= 60)
		{
			kill(cgi->getPid(), SIGKILL);
			cgi->checkWaitpid();
			for (size_t j = 0; j < _pfds.size(); j++) 
			{
        		if (_cgi_pipes.count(_pfds[j].fd) && _cgi_pipes[_pfds[j].fd] == c) 
				{
        		    cgi->closeAllOpenPipeFds();
        		    _cgi_pipes.erase(_pfds[j].fd);
        		    _pfds.erase(_pfds.begin() + j);
        		    if (j < i) i--;
        		    break;
        		}
    		}
			c->setLogMsg("CGI timeout");
			c->buildErrorResponse(504);
			_pfds[i].events = POLLOUT;
		}
	}
}

bool	ServerManager::isServerSocket(size_t pos) { return (pos < _servers.size()); }

/* ---------------------------- Internal: Error ----------------------------- */
void	ServerManager::handleInitOrAcceptError(int fd, const std::string& msg)
{
	if (fd != -1)
		close(fd);
	throw std::runtime_error(msg);
}

/* ------------------------------- Lifecycle -------------------------------- */
bool	ServerManager::_running = true;

ServerManager::ServerManager() {}

ServerManager::~ServerManager()
{
	for (client_it it = _clients.begin(); it != _clients.end(); it++)
		delete it->second;
	for (size_t i = 0; i < _servers.size(); i++)
		close(_servers[i].getSocketFd());
}

/* -------------------------------- Getters --------------------------------- */
std::vector<ServerConfig>&	ServerManager::getServers()	{ return _servers; }
size_t						ServerManager::size()		{ return _servers.size(); }

/* --------------------------------- Setup ---------------------------------- */
void	ServerManager::addServer(const ServerConfig& server) { _servers.push_back(server); }

void	ServerManager::setupServers()
{
	for (size_t i = 0; i < _servers.size(); i++)
	{
		try
		{
			int	fd = socket(AF_INET, SOCK_STREAM, 0);
			if (fd == -1)
				throw std::runtime_error("socket failed: " + std::string(strerror(errno)));
			if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
				handleInitOrAcceptError(fd, "fcntl failed: " + std::string(strerror(errno)));
			int	optval = 1;
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
				handleInitOrAcceptError(fd, "setsockopt failed: " + std::string(strerror(errno)));
			struct sockaddr_in addr = {};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(_servers[i].getPort());
			in_addr_t ip = inet_addr(_servers[i].getHost().c_str());
			if (ip == INADDR_NONE)
				handleInitOrAcceptError(fd, "invalid host: " + _servers[i].getHost());
			addr.sin_addr.s_addr = ip;
			if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
				handleInitOrAcceptError(fd, "bind failed: " + std::string(strerror(errno)));
			if (listen(fd, 128) == -1)
				handleInitOrAcceptError(fd, "listen failed: " + std::string(strerror(errno)));
			_servers[i].setSocketFd(fd);
			struct pollfd pfd = {};
			pfd.fd = fd;
			pfd.events = POLLIN;
			_pfds.push_back(pfd);
		}
		catch (const std::exception& e)
		{
			std::cerr << "\033[31m[ERROR]: Server: " << _servers[i].getServerName()
				<< ". " << e.what() << "\033[0m\n";
			_servers.erase(_servers.begin() + i);
			i--;
		}
	}
	if (_pfds.empty())
		throw std::runtime_error("no servers available, cannot continue");
}

/* -------------------------------- Runtime --------------------------------- */
void	ServerManager::start()
{
	for (size_t i = 0; i < _servers.size(); i++)
		std::cout << "\033[33m[Server: " << _servers[i].getServerName() << ":"
		<< _servers[i].getHost() << ":" << _servers[i].getPort()
		<< "] Server running...\033[0m" << std::endl;
	while (_running)
	{
		int	timeout = 60000;
		if (hasCgiClients())
			timeout = 10000;
		int	num_events = poll(&_pfds[0], _pfds.size(), timeout);
		if (num_events == -1)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll failed " + std::string(strerror(errno)));
		}
		if (num_events > 0)
			handleEvent();
		monitorClients();
	}
}

/* --------------------------------- Signal --------------------------------- */
void	ServerManager::handleSignal(int sig)
{
	(void)sig;
	const char msg[] = "\033[31m\n[INFO]: Server stopped by signal\033[0m\n";
	write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	_running = false;
}
