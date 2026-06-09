#include "../../include/ServerConfig.hpp"

ServerConfig::ServerConfig()
    : _port(80),
      _host("0.0.0.0"),
      _server_name(""),
      _root("/var/www/html"),
      _index("index.html"),
      _client_max_body_size(1048576),
      _socket_fd(-1)
{
}
ServerConfig::ServerConfig(const ServerConfig& other)
	: _port(other._port),
	  _host(other._host),
	  _server_name(other._server_name),
	  _root(other._root),
	  _index(other._index),
	  _client_max_body_size(other._client_max_body_size),
	  _error_pages(other._error_pages),
	  _locations(other._locations),
	  _socket_fd(other._socket_fd) {}

ServerConfig& ServerConfig::operator=(const ServerConfig& other)
{
	if (this != &other)
	{
		_port = other._port;
		_host = other._host;
		_server_name = other._server_name;
		_root = other._root;
		_index = other._index;
		_client_max_body_size = other._client_max_body_size;
		_error_pages = other._error_pages;
		_locations = other._locations;
		_socket_fd = other._socket_fd;
	}
	return *this;
}

ServerConfig::~ServerConfig() {}

uint16_t                   ServerConfig::getPort() const           { return _port; }
const std::string&                ServerConfig::getHost() const           { return _host; }
const std::string&                ServerConfig::getServerName() const     { return _server_name; }
const std::string&                ServerConfig::getRoot() const           { return _root; }
const std::string&                ServerConfig::getIndex() const          { return _index; }
unsigned long              ServerConfig::getClientMaxBodySize() const { return _client_max_body_size; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const     { return _error_pages; }
const std::vector<Location>&      ServerConfig::getLocations() const      { return _locations; }
int  					   ServerConfig::getSocketFd() const       { return _socket_fd; }
void ServerConfig::setPort(uint16_t port)                    { _port = port; }
void ServerConfig::setHost(const std::string &host)          { _host = host; }
void ServerConfig::setServerName(const std::string &name)    { _server_name = name; }
void ServerConfig::setRoot(const std::string &root)          { _root = root; }
void ServerConfig::setIndex(const std::string &index)        { _index = index; }
void ServerConfig::setClientMaxBodySize(unsigned long size)  { _client_max_body_size = size; }
void ServerConfig::setSocketFd(int fd) 						 { _socket_fd = fd; };
void ServerConfig::addErrorPage(int code, const std::string &page) { _error_pages[code] = page; }
void ServerConfig::addLocation(const Location &location)     { _locations.push_back(location); }


const Location* ServerConfig::findLocation(const std::string& path) const
{
	const Location*	match = NULL;
	size_t	bestLen = 0;

	for (size_t i = 0; i < _locations.size(); i++)
	{
		const std::string& locPath = _locations[i].getPath();
		if (path.find(locPath) == 0 && locPath.size() > bestLen)
		{
			size_t	len = locPath.size();
			if (path.size() == len || path[len] == '/')
			{
				bestLen = locPath.size();
				match = &_locations[i];
			}
		}
	}
	return (match);
}