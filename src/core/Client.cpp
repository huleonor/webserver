#include "../../include/Client.hpp"
#include "../../include/utils.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cerrno>
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fstream>
#include <algorithm>

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
const Response&		Client::getResponse() const 	{ return (_response); }
const std::string&	Client::getFullResponse() const		{ return (_response.getFullResponse()); }
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

static std::string	getMimeType(const std::string& path)
{
	size_t dot = path.rfind('.');
	if (dot == std::string::npos)
		return "application/octet-stream";
	std::string ext = path.substr(dot);
	if (ext == ".html" || ext == ".htm")	return "text/html";
	if (ext == ".css")						return "text/css";
	if (ext == ".js")						return "application/javascript";
	if (ext == ".json")						return "application/json";
	if (ext == ".png")						return "image/png";
	if (ext == ".jpg" || ext == ".jpeg")	return "image/jpeg";
	if (ext == ".gif")						return "image/gif";
	if (ext == ".ico")						return "image/x-icon";
	if (ext == ".txt")						return "text/plain";
	if (ext == ".pdf")						return "application/pdf";
	return "application/octet-stream";
}

void	Client::handleGet(const Location& loc)
{
	// 2. redirect
	if (!loc.getReturn().empty())
	{
		_response.setCodeStatus(301);
		_response.setStatusPhrase("Moved Permanently");
		_response.buildRedirect(loc.getReturn());
		_status = WRITING;
		return ;
	}

	struct stat	st;
	if (stat(_request.path.c_str(), &st) == 0 && S_ISDIR(st.st_mode) && _request.path[_request.path.size() - 1] != '/')
		_request.path += '/';

	if (!_request.path.empty() && _request.path[_request.path.size() - 1] == '/')
	{
		std::string	index = loc.getIndex().empty() ? _server->getIndex() : loc.getIndex();
		std::ifstream	test((_request.path + index).c_str());
		if (test.is_open())
			_request.path += index;
		else if (loc.getAutoindex())
		{
			buildAutoindex(_request.path);
			return ;
		}
		else
		{
			buildErrorResponse(403, "Forbidden");
			return ;
		}
	}

	std::ifstream	file(_request.path.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		buildErrorResponse(404, "Not Found");
		return ;
	}
	std::ostringstream	ss;
	ss << file.rdbuf();

	_response.setCodeStatus(200);
	_response.setStatusPhrase("OK");
	_response.setBody(ss.str());
	_response.buildSuccess(getMimeType(_request.path));
	_status = WRITING;
}

/* ---------------------------------- DELETE --------------------------------- */
void	Client::handleDelete()
{
	struct stat	st;
	if (stat(_request.path.c_str(), &st) == -1)
	{
		buildErrorResponse(404, "Not Found");
		return ;
	}
	if (S_ISDIR(st.st_mode))
	{
		buildErrorResponse(403, "Forbidden");
		return ;
	}
	if (remove(_request.path.c_str()) != 0)
	{
		buildErrorResponse(403, "Forbidden");
		return ;
	}
	struct in_addr tmp;
	tmp.s_addr = _client_addr;
	std::cout << "[DELETE]: " << inet_ntoa(tmp) << ":" << ntohs(_client_port)
			  << " | file: " << _request.line_request << std::endl;
	_response.setCodeStatus(204);
	_response.setStatusPhrase("No Content");
	_response.buildSuccess("");
	_status = WRITING;
}

void	Client::buildAutoindex(const std::string& dirPath)
{
	DIR*			dir = opendir(dirPath.c_str());
	if (!dir)
	{
		buildErrorResponse(403, "Forbidden");
		return ;
	}
	std::ostringstream	html;
	html << "<html><head><title>Index of " << _request.path << "</title></head>"
		 << "<body><h1>Index of " << _request.path << "</h1><hr><ul>";

	struct dirent*	entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == ".")
			continue ;
		html << "<li><a href=\"" << _request.path << name << "\">" << name << "</a></li>";
	}
	closedir(dir);
	html << "</ul><hr></body></html>";

	_response.setCodeStatus(200);
	_response.setStatusPhrase("OK");
	_response.setBody(html.str());
	_response.buildSuccess("text/html");
	_status = WRITING;
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
		struct in_addr tmp;
		tmp.s_addr = _client_addr;
		std::cout << "[REQUEST]: " << inet_ntoa(tmp) << ":" << ntohs(_client_port)
				  << " | " << _request.line_request << std::endl;
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
		return ;
	}
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
			return ;
		}
		if (_request_buffer.size() < pos + 2 + chunk_size + 2)
			return ;
		_request.body += _request_buffer.substr(pos + 2, chunk_size);
		_request_buffer.erase(0, pos + 2 + chunk_size + 2);
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

void	Client::handleCGI(const Location& loc)
{
	size_t	dot = _request.path.rfind('.');
	if (dot == std::string::npos)
	{
		buildErrorResponse(403, "Forbidden");
		return ;
	}
	std::string	ext = _request.path.substr(dot);
	const std::vector<std::string>&	cgi_exts = loc.getCgiExt();
	if (std::find(cgi_exts.begin(), cgi_exts.end(), ext) == cgi_exts.end())
	{
		buildErrorResponse(403, "Forbidden");
		return ;
	}
}

void	Client::handlePost(const Location& loc)
{
	if (_request.uploads.size() == 0)
		buildUploadFromPath(loc);
	if (_status == ERROR)
		return ;
	if (loc.getUploadPath().empty())
		return buildErrorResponse(403, "Forbidden");
	else
		_request.path = loc.getUploadPath();
	if (!isValidDirPath())
		return ;
	postContent();
	_response.setCodeStatus(201);
	_response.setStatusPhrase("Created");
	_response.setBody("");
	_response.buildSuccess("text/html");
	_status = WRITING;
}

void	Client::buildUploadFromPath(const Location& loc)
{
	UploadFile	upload;
	size_t	pos = _request.path.find_last_of('/');

	if (pos != std::string::npos)
		upload.filename = _request.path.substr(pos + 1);
	upload.content = _request.body;
	if (upload.filename == loc.getPath().substr(1) ||
		upload.filename.empty() || upload.content.empty())
			return buildErrorResponse(400, "Bad Request");
	if (pos != std::string::npos)
		_request.path.erase(pos);
	_request.uploads.push_back(upload);
}

bool	Client::isValidDirPath()
{
	struct	stat st;
	int		code = 0;
	
	if (stat(_request.path.c_str(), &st) == -1)
	{
		if (errno == EACCES)
			code = 403;
		else if (errno == ENOENT)
			code = 404;
		else
			code = 500;
	}
	else
	{
		if (!S_ISDIR(st.st_mode))
			code = 404;
		if (!(st.st_mode & S_IWUSR) || !(st.st_mode & S_IXUSR))
			code = 403;
	}
	if (code == 403)
		buildErrorResponse(403, "Forbidden");
	else if (code == 404)
		buildErrorResponse(404, "Not Found");
	else if (code == 500)
		buildErrorResponse(500, "Internal Server Error");
	return (code == 0);
}

void	Client::postContent()
{
	for (size_t i = 0; i < _request.uploads.size(); i++)
	{
		std::string		fullPath = _request.path + '/' +  _request.uploads[i].filename;
		std::ofstream	file(fullPath.c_str());

		if (!file.is_open())
			throw std::runtime_error("failed to open file");
		file.write(_request.uploads[i].content.c_str(), _request.uploads[i].content.size());
		struct in_addr tmp;
		tmp.s_addr = _client_addr;
		std::cout << "[UPLOAD]: " << inet_ntoa(tmp) << ":" << ntohs(_client_port)
				<< " | file: " << _request.uploads[i].filename
				<< " | size: " << _request.uploads[i].content.size() << " bytes" 
				<< std::endl;
	}
}

void	Client::validateAndReplacePath(const Location& loc)
{
	std::string	root = loc.getRoot().empty() ? _server->getRoot() : loc.getRoot();
	normalizeSlash(root);
	_request.path = root + _request.path;
	normalizeSlash(_request.path);
	if (!_request.isWithinRoot())
		buildErrorResponse(403, "Forbidden");
}
