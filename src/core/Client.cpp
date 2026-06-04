#include "../../include/Client.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>

/* ----------------------- Constructor and Destructor ----------------------- */
Client::Client(ServerConfig& server, int fd, struct sockaddr_in& addr)
    : _client_socket(fd),
      _request_buffer(),
      _request(),
      _content_length(0),
      _server(server),
      _client_addr(addr.sin_addr.s_addr),
      _client_port(addr.sin_port),
      _status(READING_HEADER)
{
}

Client::~Client() {};

/* --------------------------------- Getters -------------------------------- */
int					Client::getClientSocket() const { return _client_socket; }
in_port_t			Client::getClientPort() const  { return _client_port; }
in_addr_t			Client::getClientAddr() const  { return _client_addr; }
Client::Status		Client::getStatus() const { return _status; }

/* --------------------------------- Setters -------------------------------- */
void	Client::setStatus(Status status)	{ _status = status; }

/* -------------------------------- Response -------------------------------- */
void	Client::buildErrorResponse()
{
	_response.buildError(_server);
	std::cout << _response.getFullResponse() << std::endl; //// delete
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
		_request.parse(header);
		if (_request.error_code != 0)
		{
			_status = ERROR;
			_response.setCodeStatus(_request.error_code);
			if (_request.error_code == 501)
				_response.setStatusPhrase("Not Implemented");
			else if (_request.error_code == 414)
				_response.setStatusPhrase("URI Too Long");
			else
				_response.setStatusPhrase("Bad Request");
			return ;
		}
		if (_request.headers.count("content-length"))
		{
			std::istringstream ss(_request.headers["content-length"]);
			ss >> _content_length;
			if (_content_length > 0)
				_status = READING_BODY;
			else
				_status = WRITING;
		}
		else if (_request.headers.count("transfer-encoding"))
			_status = READING_BODY;
		else
			_status = WRITING;
	}
}

void	Client::receiveBody(const std::string& request)
{
	if (_request_buffer.size() + request.size() > _server.getClientMaxBodySize())
	{
		_status = ERROR;
		_response.setCodeStatus(413);
		_response.setStatusPhrase("Content Too Large");
		return ;
	}
	_request_buffer.append(request);
	if (_request_buffer.size() >= _content_length)
	{
		_request.body = _request_buffer.substr(0, _content_length);
		_status = WRITING;
	}
}
