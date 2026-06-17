#include "../../include/CgiHandler.hpp"
#include "../../include/Client.hpp"
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <iostream>

/* -------------------------------- Lifecycle ------------------------------- */
CgiHandler::CgiHandler(HttpRequest& request,  Client& client)
			: _pid(-1),
			_request(request),
			_client(client)
{
	_pipe_body[0] = -1;
	_pipe_body[1] = -1;
	_pipe_output[0] = -1;
	_pipe_output[1] = -1;
}

CgiHandler::~CgiHandler() {}

/* --------------------------------- Getters -------------------------------- */
const std::string&	CgiHandler::getExt() const			{ return (_ext); }
const std::string&	CgiHandler::getFilename() const		{ return (_filename); }
const std::string&	CgiHandler::getScriptName() const	{ return (_script_name); }
const std::string&	CgiHandler::getPathInfo() const		{ return (_path_info); }

/* ------------------------------- CGI Parsing ------------------------------ */
void	CgiHandler::extractCgiInfo(const std::string& loc)
{
	size_t root = _request.path.find(loc);
	if (root != std::string::npos) 
	{
    	size_t dot = _request.path.find(".", root);
    	if (dot == std::string::npos) 
			return _client.buildErrorResponse(403);
    	size_t start_script = root + loc.size();
    	if (start_script < _request.path.size() && _request.path[start_script] == '/')
    	    start_script++;
    	size_t next_slash = _request.path.find('/', dot);
    	size_t ext_len = (next_slash == std::string::npos) ? _request.path.length() - dot : next_slash - dot;
		_ext = _request.path.substr(dot, ext_len);
    	_filename = _request.path.substr(start_script, dot - start_script);
		_script_name = loc + _filename;
    	if (next_slash != std::string::npos)
		{
    	    _path_info = _request.path.substr(next_slash);
			_request.path = _request.path.substr(0, next_slash);
		}
	}
	else
		_client.buildErrorResponse(400);
}
/* -------------------------------- CGI Setup ------------------------------- */
struct pollfd CgiHandler::cgiSetup()
{
	setupPipe();
	if (fcntl(_pipe_body[1], F_SETFL, O_NONBLOCK) == -1 || fcntl(_pipe_output[0], F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl in cgi failed");
	if (fcntl(_pipe_body[1], F_SETFD, FD_CLOEXEC) == -1 || fcntl(_pipe_output[0], F_SETFD, FD_CLOEXEC) == -1)
		throw std::runtime_error("fcntl cloexec in cgi failed");
	setEnv();
	setupChild();
	close(_pipe_body[0]); _pipe_body[0] = -1;
	close(_pipe_output[1]);  _pipe_output[1] = -1;
	_envTmp.clear();
	_env.clear();
	struct pollfd	pfd = {};
	if (_request.method == "POST" && !_request.body.empty())
	{
		pfd.fd = _pipe_body[1];
		pfd.events = POLLOUT;
		return (pfd);
	}
	close(_pipe_body[1]); _pipe_body[1] = -1;
	pfd.fd = _pipe_output[0];
	pfd.events = POLLIN;
	return (pfd);
}

void	CgiHandler::setEnv()
{
	if (!_path_info.empty())
		_envTmp.push_back("PATH_INFO=" + _path_info);
	if (!_request.query_string.empty())
		_envTmp.push_back("QUERY_STRING=" + _request.query_string);
	_envTmp.push_back("REQUEST_METHOD=" + _request.method);
	_envTmp.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_envTmp.push_back("SERVER_PROTOCOL=HTTP/1.1");
	_envTmp.push_back("SCRIPT_NAME=" + _script_name);
	_envTmp.push_back("SERVER_NAME=" + _client.getClientServer()->getServerName());
	_envTmp.push_back("REDIRECT_STATUS=200");
	if (_request.method == "POST")
	{
		if (_request.headers.count("content-type"))
			_envTmp.push_back("CONTENT_TYPE=" + _request.headers["content-type"]);
		if (_request.headers.count("content-length"))
			_envTmp.push_back("CONTENT_LENGTH=" + _request.headers["content-length"]);
	}
	for (size_t i = 0; i < _envTmp.size(); i++)
		_env.push_back(_envTmp[i].c_str());
	_env.push_back(NULL);
}
/* ----------------------- CGI Setup (private methods) ---------------------- */
void	CgiHandler::setupPipe()
{
	if (pipe(_pipe_body) < 0)
		throw std::runtime_error("pipe body failed");
	if (pipe(_pipe_output) < 0)
	{
		close(_pipe_body[0]); _pipe_body[0] = -1;
		close(_pipe_body[1]); _pipe_body[1] = -1;
		throw std::runtime_error("pipe output failed");
	}
}

void	CgiHandler::setupChild()
{
	_pid = fork();
	if (_pid < 0)
	{
		close(_pipe_body[0]);	_pipe_body[0] = -1;
		close(_pipe_body[1]);	_pipe_body[1] = -1;
		close(_pipe_output[0]);	_pipe_output[0] = -1;
		close(_pipe_output[1]);	_pipe_output[1] = -1;
		throw std::runtime_error("fork failed");
	}
	if (_pid == 0)
	{
		dup2(_pipe_body[0], STDIN_FILENO);
		dup2(_pipe_output[1], STDOUT_FILENO);
		close(_pipe_body[0]); _pipe_body[0] = -1;
		close(_pipe_body[1]); _pipe_body[1] = -1;
		close(_pipe_output[0]); _pipe_output[0] = -1;
		close(_pipe_output[1]); _pipe_output[1] = -1;
		//////////////
		char* argv[] = {(char*)"non", (char*)"non", NULL};
		if (execve("/usr/b", argv, const_cast<char* const *>(_env.data())) < 0)
		{
			_envTmp.clear();
			_env.clear();
		}
		////////////
		exit(1);
	}
}
