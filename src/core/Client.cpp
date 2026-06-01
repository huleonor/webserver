#include "../../include/Client.hpp"
#include <stdexcept> 
#include <iostream>
/* ----------------------- Constructor and Destructor ----------------------- */
Client::Client(ServerConfig& server, int fd, struct sockaddr_in& addr)
    : _client_socket(fd),           
      _request_buffer(),            
      _response_buffer(),           
      _request(),                   
      _server(server),             
      _client_addr(addr.sin_addr.s_addr), 
      _client_port(addr.sin_port),
	  _status(READING_HEADER)
{
}

Client::~Client() {};

/* --------------------------------- Getters -------------------------------- */
in_port_t	Client::getClientPort() const  { return _client_port; }
in_addr_t	Client::getClientAddr() const  { return _client_addr; }
Client::Status		Client::getStatus() const { return _status; }

/* --------------------------------- Methods -------------------------------- */
void	Client::receiveHeader(const std::string& request)
{
	// if (_request_buffer.size() + request.size() > Client::MAX_HEADER_SIZE)
		/// to resolv
	_request_buffer.append(request);
	size_t	pos = _request_buffer.find("\r\n\r\n");
	if (pos != std::string::npos)
	{
		std::string	header = _request_buffer.substr(0, pos);
		_request_buffer.erase(0, pos + 4);
		//// PARSING - Hugo needs to change status
	}
}

void	Client::receiveBody(const std::string& request)
{
	// if (_request_buffer.size() + request.size() > _server.getClientMaxBodySize())
	// 	// send error page - to resolv
	_request_buffer.append(request);
}
