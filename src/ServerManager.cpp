#include "include/ServerManager.hpp"
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cerrno>
#include <cstring>

ServerManager::ServerManager() {}

ServerManager::~ServerManager() {}

void	ServerManager::setupServers()
{
	for (size_t i = 0; i < _servers.size(); i++)
	{
		int	fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
			throw std::runtime_error("socket failed: " +  std::string(strerror(errno)));
		if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        	throw std::runtime_error("fcntl failed: " + std::string(strerror(errno)));
		int	optval = 1;
		if (setsockopt(fd,  SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
			throw std::runtime_error("setsockopt failed: " +  std::string(strerror(errno)));
		struct	sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(_servers[i].getPort());
    	in_addr_t ip = inet_addr(_servers[i].getHost().c_str());
		if (ip == INADDR_NONE)
		    throw std::runtime_error("invalid host: " + _servers[i].getHost());
		addr.sin_addr.s_addr = ip;
		if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
			throw std::runtime_error("bind failed: " +  std::string(strerror(errno)));
		if (listen(fd, 128) == -1)
			throw std::runtime_error("listen failed: " +  std::string(strerror(errno)));
		_servers[i].setSocketFd(fd);
	}
}