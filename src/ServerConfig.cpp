#include "include/ServerConfig.hpp"

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
ServerConfig::~ServerConfig() {}

uint16_t                   ServerConfig::getPort() const           { return _port; }
std::string                ServerConfig::getHost() const           { return _host; }
std::string                ServerConfig::getServerName() const     { return _server_name; }
std::string                ServerConfig::getRoot() const           { return _root; }
std::string                ServerConfig::getIndex() const          { return _index; }
unsigned long              ServerConfig::getClientMaxBodySize() const { return _client_max_body_size; }
std::map<int, std::string> ServerConfig::getErrorPages() const     { return _error_pages; }
std::vector<Location>      ServerConfig::getLocations() const      { return _locations; }
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


