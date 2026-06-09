#include "../../include/Client.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cerrno>
#include <unistd.h>
#include <cstring>

/* ----------------------- Constructor and Destructor ----------------------- */
Client::Client(int fd, struct sockaddr_in& addr, ServerConfig& server)
    : _client_socket(fd),
      _client_addr(addr.sin_addr.s_addr),
      _client_port(addr.sin_port),
      _server(&server),
      _request_buffer(),
      _request(),
      _content_length(0),
      _chunked(false),
      _status(READING_HEADER),
      _bytes_sent(0),
	  _last_time_activity(time(NULL))
{
}

Client::~Client() { close(_client_socket); }

/* --------------------------------- Setters -------------------------------- */
void	Client::setStatus(Status status)	{ _status = status; }
void	Client::setBytesSent(ssize_t n)		{ _bytes_sent = n; }
void	Client::setLastTimeActivity(time_t time)	{ _last_time_activity = time; }

/* --------------------------------- Getters -------------------------------- */
int					Client::getClientSocket() const	{ return (_client_socket); }
in_port_t			Client::getClientPort() const	{ return (_client_port); }
in_addr_t			Client::getClientAddr() const	{ return (_client_addr); }
Client::Status		Client::getStatus() const		{ return (_status); }
const HttpRequest&		Client::getRequest() const		{ return (_request); }
const std::string&	Client::getResponse() const		{ return (_response.getFullResponse()); }
const ServerConfig*	Client::getClientServer() const { return (_server); }
ssize_t				Client::getBytesSent() const	{ return (_bytes_sent); }
time_t				Client::getLastTimeActivity() const { return (_last_time_activity); }

/* -------------------------------- Response -------------------------------- */
void	Client::buildErrorResponse(int code, const std::string& phrase)
{
	_response.setCodeStatus(code);
	_response.setStatusPhrase(phrase);
	_response.buildError(*_server);
	_status = ERROR;
}

void	Client::sendResponse()
{
	_status = WRITING;
	size_t		responseSize = _response.getFullResponse().size();
	size_t		bufferSize = responseSize - _bytes_sent;
	const char*	buff = _response.getFullResponse().c_str() + _bytes_sent;
	ssize_t		n = send(_client_socket, buff, bufferSize, 0);
	if (n == -1)
	{
		_status = ERROR;
		return ;
	}
	if ((size_t)(_bytes_sent += n) >= responseSize)
		_status = CLOSE;
}

/* --------------------------------- Request Handling ----------------------- */
ssize_t	Client::receiveData()
{
	char	buffer[4096] = {0};
	ssize_t	n = recv(_client_socket, buffer, sizeof(buffer), 0);

	if (n > 0)
	{
		if (_status == READING_HEADER)
		{
			receiveHeader(std::string(buffer, n));
			if (_status == READING_BODY)
				receiveBody("");
		}
		else if (_status == READING_BODY)
			receiveBody(std::string(buffer, n));
	}
	return (n);
}

void	Client::receiveHeader(const std::string& request)
{
	if (_request_buffer.size() + request.size() > Client::MAX_HEADER_SIZE)
	{
		buildErrorResponse(400, "Request Header Or Cookie Too Large");
		return ;
	}
	_request_buffer.append(request);
	size_t	pos = _request_buffer.find("\r\n\r\n");
	if (pos != std::string::npos)
	{
		std::string	header = _request_buffer.substr(0, pos);
		_request_buffer.erase(0, pos + 4);
		_request.parse(header);
		if (_request.error_code != 0)
		{
			if (_request.error_code == 501)
				buildErrorResponse(501, "Not Implemented");
			else if (_request.error_code == 414)
				buildErrorResponse(414, "URI Too Long");
			else
				buildErrorResponse(400, "Bad Request");
			return ;
		}
		if (_request.headers.count("content-length"))
		{
			std::istringstream ss(_request.headers["content-length"]);
			ss >> _content_length;
			if (_content_length > _server->getClientMaxBodySize())
			{
				buildErrorResponse(413, "Content Too Large");
				return ; 
			}
			_status = (_content_length > 0) ? READING_BODY : PROCESSING;
		}
		else if (_request.headers.count("transfer-encoding"))
		{
			_chunked = true;
			_status = READING_BODY;
		}
		else
			_status = PROCESSING;
	}
}

void	Client::receiveBody(const std::string& request)
{
	if (_request_buffer.size() + request.size() > _server->getClientMaxBodySize())
	{
		buildErrorResponse(413, "Content Too Large");
		return ;
	}
	_request_buffer.append(request);
	if (!_chunked)
	{
		if (hasCompleteBody())
		{
			_status = PROCESSING;
			return ;
		}
	}
	else
	{
		// chunked transfer encoding
		while (!_request_buffer.empty())
		{
			size_t pos = _request_buffer.find("\r\n");
			if (pos == std::string::npos)
				return ;
			size_t chunk_size;
			std::istringstream ss(_request_buffer.substr(0, pos));
			ss >> std::hex >> chunk_size;
			if (chunk_size == 0)
			{
				_status = PROCESSING;
				// tmp: verify chunked body was assembled correctly
				std::cout << "[DEBUG] chunked body complete: \"" << _request.body << "\"" << std::endl;
				return ;
			}
			if (_request_buffer.size() < pos + 2 + chunk_size + 2)
				return ;
			_request.body += _request_buffer.substr(pos + 2, chunk_size);
			// tmp: print each chunk as it's extracted
			std::cout << "[DEBUG] chunk extracted: \"" << _request_buffer.substr(pos + 2, chunk_size) << "\"" << std::endl;
			_request_buffer.erase(0, pos + 2 + chunk_size + 2);
		}
	}
}

bool	Client::hasCompleteBody()
{
	if (_content_length > 0 && _request_buffer.size() >= _content_length)
	{
		_request.body = _request_buffer.substr(0, _content_length);
		return (true);
	}
	return (false);
}
