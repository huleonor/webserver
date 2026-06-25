#include "../../include/Client.hpp"
#include "../../include/utils.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <algorithm>

/* --------------------------------- Static --------------------------------- */
static std::string	getMimeType(const std::string& path)
{
	size_t dot = path.rfind('.');
	if (dot == std::string::npos)
		return "application/octet-stream";
	std::string ext = path.substr(dot);
	if (ext == ".html" || ext == ".htm")	return "text/html";
	if (ext == ".css")						return "text/css";
	if (ext == ".js")						return "application/javascript";
	if (ext == ".json")					return "application/json";
	if (ext == ".png")						return "image/png";
	if (ext == ".jpg" || ext == ".jpeg")	return "image/jpeg";
	if (ext == ".gif")						return "image/gif";
	if (ext == ".ico")						return "image/x-icon";
	if (ext == ".txt")						return "text/plain";
	if (ext == ".pdf")						return "application/pdf";
	return "application/octet-stream";
}

/* ---------------------------- Internal: Request --------------------------- */
void	Client::receiveHeader(const std::string& request)
{
	if (_request_buffer.size() + request.size() > Client::MAX_HEADER_SIZE)
	{
		buildErrorResponse(400);
		return ;
	}
	_request_buffer.append(request);
	size_t	pos = _request_buffer.find("\r\n\r\n");
	if (pos != std::string::npos)
	{
		std::string	header = _request_buffer.substr(0, pos);
		if (header.empty())
			return ;
		_request_buffer.erase(0, pos + 4);
		_request.parse(header);
		struct in_addr tmp;
		tmp.s_addr = _client_addr;
		std::cout << "[REQUEST]: " << inet_ntoa(tmp) << ":" << ntohs(_client_port)
				  << " | " << _request.line_request << std::endl;
		if (_request.error_code != 0)
		{
			buildErrorResponse(_request.error_code);
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
				buildErrorResponse(400);
				return ;
			}
			if (_content_length > _server->getClientMaxBodySize())
			{
				buildErrorResponse(413);
				return ;
			}
			_status = (_content_length > 0) ? READING_BODY : PROCESSING;
		}
		else
		{
			if (!_request_buffer.empty())
			{
				buildErrorResponse(411);
				return ;
			}
			_status = PROCESSING;
		}
	}
}

void	Client::receiveBody(const std::string& request)
{
	if (_request_buffer.size() + request.size() > _server->getClientMaxBodySize())
	{
		buildErrorResponse(413);
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
		if (_request.body.size() > _server->getClientMaxBodySize())
		{
			buildErrorResponse(413);
			return ;
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
		buildErrorResponse(400);
}

/* --------------------------- Internal: Response --------------------------- */
void	Client::buildAutoindex(const std::string& dirPath, const std::string& loc)
{
	DIR*	dir = opendir(dirPath.c_str());
	if (!dir)
	{
		buildErrorResponse(403);
		return ;
	}
	std::ostringstream	html;
	html << "<html><head><title>Index of " << _request.uri << "</title></head>"
		 << "<body><h1>Index of " << _request.uri << "</h1><hr><ul>";

	struct dirent*	entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == ".")
			continue ;
		html << "<li><a href=\"" << loc
			<< (loc[loc.size() - 1] == '/' ? "" : "/") << name << "\">" << name << "</a></li>";
	}
	closedir(dir);
	html << "</ul><hr></body></html>";

	_response.setCodeStatus(200);
	_response.setStatusPhrase("OK");
	_response.setBody(html.str());
	_response.buildSuccess("text/html");
	_status = WRITING;
}

void	Client::buildUploadFromPath(const Location& loc)
{
	UploadFile	upload;
	size_t	pos = _request.path.find_last_of('/');

	if (pos != std::string::npos && _request.path != loc.getUploadPath())
		upload.filename = _request.path.substr(pos + 1);
	upload.content = _request.body;
	if (upload.filename == loc.getPath().substr(1) ||
		upload.filename.empty() || upload.content.empty())
			return buildErrorResponse(400);
	if (pos != std::string::npos)
		_request.path.erase(pos);
	_request.uploads.push_back(upload);
}

void	Client::postContent()
{
	for (size_t i = 0; i < _request.uploads.size(); i++)
	{
		std::string		fullPath = _request.path + '/' + _request.uploads[i].filename;
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

/* ------------------------------- Lifecycle -------------------------------- */
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
      _last_time_activity(time(NULL)),
      _cgi(NULL)
{
}

Client::~Client()
{
	close(_client_socket);
	_client_socket = -1;
	if (_cgi)
	{
		delete _cgi;
		_cgi = NULL;
	}
}

/* -------------------------------- Getters --------------------------------- */
int					Client::getClientSocket() const		{ return (_client_socket); }
in_port_t			Client::getClientPort() const		{ return (_client_port); }
in_addr_t			Client::getClientAddr() const		{ return (_client_addr); }
Client::Status		Client::getStatus() const			{ return (_status); }
const HttpRequest&	Client::getRequest() const			{ return (_request); }
const Response&		Client::getResponse() const			{ return (_response); }
const std::string&	Client::getFullResponse() const		{ return (_response.getFullResponse()); }
const ServerConfig*	Client::getClientServer() const		{ return (_server); }
ssize_t				Client::getBytesSent() const		{ return (_bytes_sent); }
time_t				Client::getLastTimeActivity() const	{ return (_last_time_activity); }
const std::string&	Client::getLogMsg() const			{ return (_response.getLogMsg()); }
CgiHandler*			Client::getCgi() const				{ return (_cgi); }

/* -------------------------------- Setters --------------------------------- */
void	Client::setStatus(Status status)				{ _status = status; }
void	Client::setBytesSent(ssize_t n)					{ _bytes_sent = n; }
void	Client::setLastTimeActivity(time_t time)		{ _last_time_activity = time; }
void	Client::setLogMsg(const std::string& msg)		{ _response.setLogMsg(msg); }

/* ---------------------------- Request Handling ---------------------------- */

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

/* -------------------------------- Response -------------------------------- */

void	Client::buildErrorResponse(int code)
{
	_response.setCodeStatus(code);
	_response.setStatusPhrase(getErrorPhrase(code));
	_response.buildError(*_server);
	_status = ERROR;
	std::ostringstream	oss;
	oss << "error with code: " << code << " " << getErrorPhrase(code);
	setLogMsg(oss.str());
}

void	Client::buildCgiResponse(const std::string& body, const std::string& content_type, int status_code)
{
	std::string	phrase = "OK";
	if (status_code == 201) phrase = "Created";
	else if (status_code == 204) phrase = "No Content";
	else if (status_code == 301) phrase = "Moved Permanently";
	else if (status_code == 302) phrase = "Found";
	else if (status_code == 400) phrase = "Bad Request";
	else if (status_code == 403) phrase = "Forbidden";
	else if (status_code == 404) phrase = "Not Found";
	else if (status_code == 500) phrase = "Internal Server Error";
	_response.setCodeStatus(status_code);
	_response.setStatusPhrase(phrase);
	_response.setBody(body);
	_response.buildSuccess(content_type);
	_status = WRITING;
}

void	Client::sendResponse()
{
	_status = WRITING;
	size_t		responseSize = _response.getFullResponse().size();
	size_t		bufferSize = responseSize - _bytes_sent;
	const char*	buff = _response.getFullResponse().c_str() + _bytes_sent;
	ssize_t		n = send(_client_socket, buff, bufferSize, 0);
	if (n <= 0)
	{
		_status = ERROR;
		return ;
	}
	if ((size_t)(_bytes_sent += n) >= responseSize)
		_status = CLOSE;
}

/* ------------------------------ HTTP Methods ------------------------------ */
void	Client::handleGet(const Location& loc)
{
	if (!loc.getReturn().empty())
	{
		_response.setCodeStatus(301);
		_response.setStatusPhrase("Moved Permanently");
		_response.buildRedirect(loc.getReturn());
		_status = WRITING;
		return ;
	}
	if (_request.isValidPath() && S_ISDIR(_request.target_info.st_mode) && _request.path[_request.path.size() - 1] != '/')
		_request.path += '/';

	if (!_request.path.empty() && _request.path[_request.path.size() - 1] == '/')
	{
		std::string	index = loc.getIndex().empty() ? _server->getIndex() : loc.getIndex();
		std::ifstream	test((_request.path + index).c_str());
		if (test.is_open())
			_request.path += index;
		else if (loc.getAutoindex())
		{
			buildAutoindex(_request.path, loc.getPath());
			return ;
		}
		else
		{
			buildErrorResponse(403);
			return ;
		}
	}

	std::ifstream	file(_request.path.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		buildErrorResponse(404);
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

void	Client::handlePost(const Location& loc)
{
	if (loc.getUploadPath().empty())
		return buildErrorResponse(403);
	if (_request.uploads.size() == 0)
		buildUploadFromPath(loc);
	if (_status == ERROR)
		return ;
	if (_request.isValidPath())
	{
		if (!S_ISDIR(_request.target_info.st_mode))
			_request.error_code = 404;
		if (!(_request.target_info.st_mode & S_IWUSR)
			|| !(_request.target_info.st_mode & S_IXUSR))
				_request.error_code = 403;
	}
	if (_request.error_code != 0)
		return buildErrorResponse(_request.error_code);
	postContent();
	(_request.uploads.size() > 1) ? _response.setUploadLocation(loc.getPath(), "") 
			: _response.setUploadLocation(loc.getPath(), _request.uploads[0].filename);
	_response.setCodeStatus(201);
	_response.setStatusPhrase("Created");
	_response.setBody("");
	_response.buildSuccess("text/html");
	_status = WRITING;
}

void	Client::handleDelete()
{
	struct stat	st;
	if (stat(_request.path.c_str(), &st) == -1)
	{
		buildErrorResponse(404);
		return ;
	}
	if (S_ISDIR(st.st_mode))
	{
		buildErrorResponse(403);
		return ;
	}
	if (remove(_request.path.c_str()) != 0)
	{
		buildErrorResponse(403);
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

void	Client::handleCGI(const Location& loc)
{
	_cgi = new CgiHandler(_request, *this);
	_cgi->extractCgiInfo(loc.getPath());
	if (_request.error_code != 0)
		return ;
	const std::vector<std::string>&	cgi_exts = loc.getCgiExt();
	const std::vector<std::string>&	cgi_paths = loc.getCgiPath();
	size_t	idx = std::find(cgi_exts.begin(), cgi_exts.end(), _cgi->getExt()) - cgi_exts.begin();
	if (idx >= cgi_exts.size())
		return buildErrorResponse(403);
	_cgi->setInterpreterPath(cgi_paths[idx]);
	if (!_request.isValidPath())
		return buildErrorResponse(_request.error_code);
}

void	Client::validateAndReplacePath(const Location& loc)
{
	std::string	root;
	if (!loc.getUploadPath().empty())
	{
		size_t	pos = _request.path.find(loc.getPath());
		pos = pos + loc.getPath().size();
		if (_request.path[pos] == '/')
			pos++;
		root = loc.getUploadPath();
		normalizeSlash(root);
		_request.path.erase(0, pos);
		_request.path = root + '/' + _request.path;
	}
	else
	{
		root = loc.getRoot().empty() ? _server->getRoot() : loc.getRoot();
		normalizeSlash(root);
		if (!loc.getRoot().empty())
		{
			std::string suffix = _request.path.substr(loc.getPath().size());
			if (suffix.empty() || suffix[0] != '/')
				suffix = "/" + suffix;
			_request.path = root + suffix;
		}
		else
			_request.path = root + _request.path;
		normalizeSlash(_request.path);
	}
	if (!_request.resolvePathWithinRoot(root))
		buildErrorResponse(403);
}
