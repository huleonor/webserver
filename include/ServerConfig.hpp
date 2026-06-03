#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <stdint.h>
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
	uint16_t                          	getPort() const;
	const std::string&					getHost() const;
	const std::string&					getServerName() const;
	const std::string&					getRoot() const;
	const std::string&					getIndex() const;
	unsigned long                     	getClientMaxBodySize() const;
	const std::map<int, std::string>&	getErrorPages() const;
	int                               	getSocketFd() const;
	const std::vector<Location>&		getLocations() const;
	
	void setPort(uint16_t port);
	void setHost(const std::string &host);
	void setServerName(const std::string &server_name);
	void setRoot(const std::string &root);
	void setIndex(const std::string &index);
	void setClientMaxBodySize(unsigned long size);
	void setSocketFd(int fd);
	void addErrorPage(int code, const std::string &page);
	void addLocation(const Location &location);
};

#endif
