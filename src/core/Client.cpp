#include "../../include/Client.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cerrno>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <fstream>

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
const HttpRequest&	Client::getRequest() const		{ return (_request); }
const std::string&	Client::getResponse() const		{ return (_response.getFullResponse()); }
const ServerConfig*	Client::getClientServer() const { return (_server); }
ssize_t				Client::getBytesSent() const	{ return (_bytes_sent); }
time_t				Client::getLastTimeActivity() const { return (_last_time_activity); }

/* -------------------------------- Response -------------------------------- */
void	Client::parseMultipartIfNeeded()
{
	std::map<std::string, std::string>::iterator it = _request.headers.find("content-type");
	if (it == _request.headers.end())
		return ;
	std::string& content_type = it->second;
	if (content_type.find("multipart/form-data") == std::string::npos)
		return ;
	size_t boundary_pos = content_type.find("boundary=");
	if (boundary_pos == std::string::npos)
		return ;
	std::string boundary = content_type.substr(boundary_pos + 9);
	_request.parseMultipart(boundary);
	if (_request.uploads.size() == 0)
		buildErrorResponse(400, "Bad Request");
}

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
		if (_request.headers.count("transfer-encoding"))
		{
			_chunked = true;
			_status = READING_BODY;
		}
		else if (_request.headers.count("content-length"))
		{
			std::istringstream ss(_request.headers["content-length"]);
			ss >> _content_length;
			if (ss.fail())
			{
				buildErrorResponse(400, "Bad Request");
				return ;
			}
			if (_content_length > _server->getClientMaxBodySize())
			{
				buildErrorResponse(413, "Content Too Large");
				return ;
			}
			_status = (_content_length > 0) ? READING_BODY : PROCESSING;
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
			parseMultipartIfNeeded();
			if (_status != ERROR)
				_status = PROCESSING;
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
				parseMultipartIfNeeded();
				if (_status != ERROR)
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

void	Client::handlePost(const Location& loc)
{
	buildUploadFromPath(loc);
	if (_request.uploads.size() == 0)
		return ;

	std::string	dirPath;
	std::string	root = loc.getRoot().empty() ? _server->getRoot() : loc.getRoot();
	if (_request.path == loc.getPath() && loc.getUploadPath().empty() == false)
		dirPath = loc.getUploadPath();
	else
		dirPath =  (loc.getRoot().empty() ? _server->getRoot() : loc.getRoot()) + _request.path;
	struct	stat st;

	if (stat(dirPath.c_str(), &st) == 0)
	{
		if (S_ISDIR(st.st_mode))
			std::cout << dirPath << " is dir" << std::endl;
		else if (S_ISREG(st.st_mode))
			std::cout << _request.uploads[0].filename << " is file" << std::endl;
	}
	else
		std::cout << dirPath << " not exist" << std::endl;
	
}

void	Client::buildUploadFromPath(const Location& loc)
{
	size_t	pos = 0;
	if (_request.uploads.size() == 0)
	{
		pos = _request.path.find_last_of('/');
		std::string filename = _request.path.substr(pos + 1);
		std::string	locName = loc.getPath().substr(1);
		if (filename == locName || filename.empty())
			return buildErrorResponse(400, "Bad Request");
		_request.path = _request.path.erase(pos);
		UploadFile	upload;
		upload.filename = filename;
		upload.content = _request.body;
		_request.uploads.push_back(upload);
	}
	else
	{
		pos = _request.path.size() - 1;
		if (_request.path[pos] == '/')
			_request.path.erase(pos);
	}
}
