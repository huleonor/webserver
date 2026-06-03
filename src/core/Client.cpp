#include "../../include/Client.hpp"
#include <stdexcept> 
#include <iostream>
#include <cerrno>  

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
void	Client::setBytesSent(ssize_t n)		{ _bytes_sent = n; }

/* -------------------------------- Response -------------------------------- */
void	Client::buildErrorResponse(int code, const std::string& phrase)
{
	_response.setCodeStatus(code);
	_response.setStatusPhrase(phrase);
	_response.buildError(_server);
}

void	Client::sendResponse()
{
	_status = WRITING;
	size_t		responseSize = _response.getFullResponse().size();
	size_t		bufferSize = responseSize - _bytes_sent;
	const char*	buff = _response.getFullResponse().c_str() + _bytes_sent;
	ssize_t	n = send(_client_socket, buff, bufferSize, 0);
	if (n == -1)
	{
		_status = ERROR;
		return ;
	}
	if ((size_t)(_bytes_sent += n) >= responseSize)
		_status = CLOSE;
}

/* --------------------------------- Request Handling -------------------------------- */
ssize_t	Client::receiveData()
{
	char	buffer[4096] = {0};
	ssize_t	n = recv(_client_socket, buffer, sizeof(buffer), 0);

	if (n > 0)
	{
		if (_status == READING_HEADER)
			receiveHeader(std::string(buffer, n));
		else
			receiveBody(std::string(buffer, n));
	}
	return (n);
}

void	Client::receiveHeader(const std::string& request)
{
	if (_request_buffer.size() + request.size() > Client::MAX_HEADER_SIZE)
	{
		_status = ERROR;
		buildErrorResponse(400, "Request Header Or Cookie Too Large");
		return ;
	}
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
