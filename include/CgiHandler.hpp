#ifndef CGI_HANDLER
# define CGI_HANDLER

#include <string>
#include <unistd.h>
#include "HttpRequest.hpp"

class Client;

class CgiHandler
{
private:
// Attributes
	int				_pipe_body[2];
	int				_pipe_output[2];
	pid_t			_pid;
	char**			_env;
	HttpRequest&	_request;
	Client&			_client;
	std::string		_ext;
	std::string		_filename;
	std::string 	_script_name;
	std::string		_path_info;

// Non-copyable (owns a unique socket fd)
	CgiHandler(const CgiHandler& other);
	CgiHandler&	operator=(const CgiHandler& other);

public:
// Lifecycle
	CgiHandler(HttpRequest& request, Client& client);
	~CgiHandler();

// Getters
	const std::string&	getExt() const;
	const std::string&	getFilename() const;
	const std::string&	getScriptName() const;
	const std::string&	getPathInfo() const;

// CGI Parsing
	void	extractCgiInfo(const std::string& loc);

// CGI Setup
	void	cgiSetup();
	void	setEnv();
};


#endif