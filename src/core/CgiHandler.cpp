#include "../../include/CgiHandler.hpp"
#include "../../include/Client.hpp"
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>

/* -------------------------------- Lifecycle ------------------------------- */
CgiHandler::CgiHandler(HttpRequest& request,  Client& client) 
			: _pid(-1),
			_env(NULL),
			_request(request),
			_client(client)
{
	_pipe_body[0] = -1;
	_pipe_body[1] = -1;
	_pipe_output[0] = -1;
	_pipe_output[1] = -1;
}

CgiHandler::~CgiHandler() 
{
	if (_env)
	{
		size_t	i = 0;
		while (_env[i])
		{
			delete[] _env[i];
			i++;
		}
		delete[] _env;
	}
}

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
void	CgiHandler::cgiSetup()
{
	if (pipe(_pipe_body) < 0)
		throw std::runtime_error("pipe body failed");
	if (pipe(_pipe_output) < 0)
	{
		close(_pipe_body[0]);
		close(_pipe_body[1]);
		throw std::runtime_error("pipe output failed");
	}
	if (fcntl(_pipe_body[1], F_SETFL, O_NONBLOCK) == -1 || fcntl(_pipe_output[0], F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl in cgi failed");
	setEnv();
	_pid = fork();
	if (_pid < 0)
	{
		close(_pipe_body[0]);	_pipe_body[0] = -1;
		close(_pipe_body[1]);	_pipe_body[1] = -1;
		close(_pipe_output[0]);	_pipe_output[0] = -1;
		close(_pipe_output[1]);	_pipe_output[1] = -1;
		throw std::runtime_error("fork failed");
	}
}

void	CgiHandler::setEnv()
{
	std::vector<std::string>	env;

	if (!_path_info.empty())
		env.push_back("PATH_INFO=" + _path_info);
	if (!_request.query_string.empty())
		env.push_back("QUERY_STRING=" + _request.query_string);
	env.push_back("REQUEST_METHOD=" + _request.method);
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env.push_back("SCRIPT_NAME=" + _script_name);
	env.push_back("SERVER_NAME=" + _client.getClientServer()->getServerName());
	env.push_back("SERVER_PORT=" + _client.getClientServer()->getPort());
	env.push_back("REDIRECT_STATUS=200");
	if (_request.method == "POST")
	{
		if (_request.headers.count("content-type"))
			env.push_back("CONTENT_TYPE=" + _request.headers["content-type"]);
		if (_request.headers.count("content-length"))
			env.push_back("CONTENT_LENGTH=" + _request.headers["content-length"]);
	}
	_env = new char*[env.size() + 1];
	for (size_t i = 0; i < env.size(); i++)
	{
		(_env)[i] = new char[env[i].size() + 1];
		std::copy(env[i].begin(), env[i].end(), (_env)[i]);
		(_env)[i][env[i].size()] = '\0';
	}
	(_env)[env.size()] = NULL;
}
