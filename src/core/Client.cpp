#include "../../include/Client.hpp"
#include <stdexcept> 
#include <iostream>

/* ----------------------- Constructor and Destructor ----------------------- */
Client::Client(int fd, struct sockaddr_in& addr, ServerConfig& server)
    : _client_socket(fd),           
	_client_addr(addr.sin_addr.s_addr), 
	_client_port(addr.sin_port),
	_server(server),             
    _request_buffer(),                       
    _request(),                   
	_status(READING_HEADER),
	_bytes_sent(0)
{
}

Client::~Client() {};

/* --------------------------------- Getters -------------------------------- */
int					Client::getClientSocket() const 	{ return (_client_socket); }
in_port_t			Client::getClientPort() const  		{ return (_client_port); }
in_addr_t			Client::getClientAddr() const  		{ return (_client_addr); }
Client::Status		Client::getStatus() const			{ return (_status); }
const std::string&	Client::getResponse() const			{ return (_response.getFullResponse()); }
ssize_t				Client::getBytesSent() const		{ return (_bytes_sent); }

/* --------------------------------- Setters -------------------------------- */
void	Client::setStatus(Status status)	{ _status = status; }
void	Client::setBytesSent(ssize_t n)			{ _bytes_sent = n; }

/* -------------------------------- Response -------------------------------- */
void	Client::buildErrorResponse()
{
	_response.buildError(_server);
}

void	Client::sendResponse()
{
	if (_status == WRITING)
		_status	= WRITING;
	size_t		responseSize = _response.getFullResponse().size();
	size_t		bufferSize = responseSize - _bytes_sent;
	const char*	buff = _response.getFullResponse().c_str() + _bytes_sent;
	ssize_t	n = send(_client_socket, buff, bufferSize, 0);
	if (n == -1 || (size_t)(_bytes_sent += n) >= responseSize)
		_status = CLOSE;
}

/* --------------------------------- Request Handling -------------------------------- */
void	Client::receiveHeader(const std::string& request)
{
	if (_request_buffer.size() + request.size() > Client::MAX_HEADER_SIZE)
	{
		_status = ERROR;
		_response.setCodeStatus(400);
		_response.setStatusPhrase("Request Header Or Cookie Too Large");
		return ;
	}
	_request_buffer.append(request);
	size_t	pos = _request_buffer.find("\r\n\r\n");
	if (pos != std::string::npos)
	{
		std::string	header = _request_buffer.substr(0, pos);
		_request_buffer.erase(0, pos + 4);
		std::cout << "\033[32mClient: " << _client_addr << ":" << _client_port
			<< " | header received\033[0m" << std::endl;
		//// PARSING - Hugo needs to change status
	}
}

void	Client::receiveBody(const std::string& request)
{
	// if (_request_buffer.size() + request.size() > _server.getClientMaxBodySize())
	// 	// send error page - to resolv
	_request_buffer.append(request);
}
