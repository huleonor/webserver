#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include "Location.hpp"

class ServerConfig
{
private:
	uint16_t                   _port;
	std::string                _host;
	std::string                _server_name;
	std::string                _root;
	std::string                _index;
	unsigned long              _client_max_body_size;
	std::map<int, std::string> _error_pages;
	std::vector<Location>      _locations;
	int                        _socket_fd;

public:
	ServerConfig();
	~ServerConfig();

	uint16_t                          getPort() const;
	std::string                       getHost() const;
	std::string                       getServerName() const;
	std::string                       getRoot() const;
	std::string                       getIndex() const;
	unsigned long                     getClientMaxBodySize() const;
	std::map<int, std::string>        getErrorPages() const;
	int                               getSocketFd() const;
	void                              setSocketFd(int fd);
	std::vector<Location>             getLocations() const;

	void setPort(uint16_t port);
	void setHost(const std::string &host);
	void setServerName(const std::string &server_name);
	void setRoot(const std::string &root);
	void setIndex(const std::string &index);
	void setClientMaxBodySize(unsigned long size);
	void addErrorPage(int code, const std::string &page);
	void addLocation(const Location &location);
};

#endif
