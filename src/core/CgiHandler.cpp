#include "../../include/CgiHandler.hpp"
#include "../../include/Client.hpp"
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/wait.h>
#include <signal.h>

#include <iostream>

/* ----------------------------- Internal: Setup ---------------------------- */
void	CgiHandler::setEnv()
{
	if (!_path_info.empty())
		_envTmp.push_back("PATH_INFO=" + _path_info);
	if (!_request.query_string.empty())
		_envTmp.push_back("QUERY_STRING=" + _request.query_string);
	_envTmp.push_back("REQUEST_METHOD=" + _request.method);
	_envTmp.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_envTmp.push_back("SERVER_PROTOCOL=HTTP/1.1");
	_envTmp.push_back("SCRIPT_NAME=" + _script_path);
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

void	CgiHandler::setupPipe()
{
	if (pipe(_pipe_body) < 0)
		throw std::runtime_error("pipe body failed");
	if (pipe(_pipe_output) < 0)
	{
		closeAllOpenPipeFds();
		throw std::runtime_error("pipe output failed");
	}
}

void	CgiHandler::setupChild()
{
	_pid = fork();
	if (_pid < 0)
	{
		closeAllOpenPipeFds();
		throw std::runtime_error("fork failed");
	}
	if (_pid == 0)
	{
		std::string dir = ".";
		std::string filename = _script_path;
		size_t slash = _script_path.rfind('/');
		if (slash != std::string::npos)
		{
			dir = _script_path.substr(0, slash);
			filename = _script_path.substr(slash + 1);
		}
		dup2(_pipe_body[0], STDIN_FILENO);
		dup2(_pipe_output[1], STDOUT_FILENO);
		closeAllFds();
		chdir(dir.c_str());
		char* new_argv[3] = {(char*)_interpreter_path.c_str(), (char*)filename.c_str(), NULL};
		execve(new_argv[0], new_argv, const_cast<char* const *>(_env.data()));
		exit(1);
	}
}

void	CgiHandler::closeAllFds()
{
	int max_fd = sysconf(_SC_OPEN_MAX);
	for (int fd = 3; fd < max_fd; fd++)
		close(fd);
}

/* -------------------------------- Lifecycle ------------------------------- */
CgiHandler::CgiHandler(HttpRequest& request,  Client& client)
			: _status(NONE),
			_pid(-1),
			_body_bytes_sent(0),
			_start_time(time(NULL)),
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
const std::string&	CgiHandler::getScriptName() const	{ return (_script_path); }
const std::string&	CgiHandler::getPathInfo() const		{ return (_path_info); }
const std::string&	CgiHandler::getCgiOutputBuffer() const	{ return (_cgi_output_buffer); }
pid_t				CgiHandler::getPid() const			{ return (_pid); }
time_t				CgiHandler::getStartTime() const	{ return (_start_time); }
const int*			CgiHandler::getPipeBody() const			{ return (_pipe_body); }
const int*			CgiHandler::getPipeOutput() const		{ return (_pipe_output); }
CgiHandler::Status		CgiHandler::getStatus() const		{ return (_status); }

/* --------------------------------- Setters -------------------------------- */
void	CgiHandler::setStatus(Status status)				{ _status = status; }
void	CgiHandler::setInterpreterPath(const std::string& path) { _interpreter_path = path; }

/* ------------------------------- CGI Parsing ------------------------------ */
void	CgiHandler::extractCgiInfo(const std::string& loc)
{
	(void)loc;
	size_t search_start = (_request.path.size() >= 2 && _request.path[0] == '.' && _request.path[1] == '/') ? 2 : 0;
	size_t dot = _request.path.find('.', search_start);
	if (dot == std::string::npos)
		return _client.buildErrorResponse(403);
	size_t next_slash = _request.path.find('/', dot);
	size_t ext_len = (next_slash == std::string::npos) ? _request.path.length() - dot : next_slash - dot;
	_ext = _request.path.substr(dot, ext_len);
	std::string script = (next_slash == std::string::npos) ? _request.path : _request.path.substr(0, next_slash);
	if (script.size() >= 2 && script[0] == '.' && script[1] == '/')
		script.erase(0, 2);
	else if (!script.empty() && script[0] == '/')
		script.erase(0, 1);
	_script_path = script;
	size_t slash = script.rfind('/');
	_filename = (slash != std::string::npos) ? script.substr(slash + 1) : script;
	if (next_slash != std::string::npos)
	{
		_path_info = _request.path.substr(next_slash);
		_request.path = _request.path.substr(0, next_slash);
	}
}
/* -------------------------------- CGI Setup ------------------------------- */
struct pollfd CgiHandler::cgiSetup()
{
	setupPipe();
	if (fcntl(_pipe_body[1], F_SETFL, O_NONBLOCK) == -1 || fcntl(_pipe_output[0], F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl in cgi failed");
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
	}
	else
	{
		close(_pipe_body[1]); _pipe_body[1] = -1;
		pfd.fd = _pipe_output[0];
		pfd.events = POLLIN;
	}
	return (pfd);
}

/* ------------------------------- CGI Process ------------------------------ */
void	CgiHandler::receiveCgiOutput(const std::string& buffer)	{ _cgi_output_buffer += buffer; }

void	CgiHandler::sendBody(const std::string& body)
{
	const char*	buffer = body.c_str() + _body_bytes_sent;
	size_t	buf_len = body.size() - _body_bytes_sent;
	int bytes = write(_pipe_body[1], buffer, buf_len);
	if (bytes == 0)
		return ;
	if (bytes < 0)
		{ _status = ERROR; return; }
	if (bytes > 0)
	{
		_body_bytes_sent += bytes;
		if (_body_bytes_sent >= body.size())
			_status = DONE;
	}
}

int	CgiHandler::checkWaitpid()
{
	int	status;
	int	toReturn = 0;
	pid_t	result = waitpid(_pid, &status, WNOHANG);

	if (result < 0)
		toReturn = -1;
	if (result > 0)
	{
		if (WIFEXITED(status))
			(WEXITSTATUS(status) == 0) ? toReturn = 0 : toReturn = -1;
		else
			toReturn = -1;
	}
	else if (result == 0)
	{
		kill(_pid, SIGKILL);
		waitpid(_pid, &status, 0);
		toReturn = -1;
	}
	return (toReturn);
}

void	CgiHandler::closeAllOpenPipeFds()
{
	if (_pipe_body[1] != -1)
		{ close(_pipe_body[1]); _pipe_body[1] = -1; };
	if (_pipe_body[0] != -1)
		{ close(_pipe_body[0]); _pipe_body[0] = -1; };
	if (_pipe_output[0] != -1)
		{ close(_pipe_output[0]); _pipe_output[0] = -1; };
	if (_pipe_output[1] != -1)
		{ close(_pipe_output[1]); _pipe_output[1] = -1; };
}

void	CgiHandler::closePipeFd(int fd)
{
	if (fd == _pipe_body[1] && _pipe_body[1] != -1)
		{ close(_pipe_body[1]); _pipe_body[1] = -1; }
	else if (fd == _pipe_body[0] && _pipe_body[0] != -1)
		{ close(_pipe_body[0]); _pipe_body[0] = -1; }
	else if (fd == _pipe_output[1] && _pipe_output[1] != -1)
		{ close(_pipe_output[1]); _pipe_output[1] = -1; }
	else if (fd == _pipe_output[0] && _pipe_output[0] != -1)
		{ close(_pipe_output[0]); _pipe_output[0] = -1; };
}
