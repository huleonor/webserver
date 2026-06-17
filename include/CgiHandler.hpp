#ifndef CGI_HANDLER
# define CGI_HANDLER

#include <string>
#include <vector>
#include <unistd.h>
#include <poll.h>
#include "HttpRequest.hpp"

class Client;

class CgiHandler
{
private:
// Attributes
	int							_pipe_body[2];
	int							_pipe_output[2];
	pid_t						_pid;
	std::string					_ext;
	std::string					_filename;
	std::string 				_script_name;
	std::string					_path_info;
	HttpRequest&				_request;
	Client&						_client;
	std::vector<std::string>	_envTmp;
	std::vector<const char*>	_env;
	std::vector<struct pollfd>&	_pfds;

// Non-copyable (owns a unique socket fd)
	CgiHandler(const CgiHandler& other);
	CgiHandler&	operator=(const CgiHandler& other);
// CGI Setup (private methods)
	void	setEnv();
	void	setupPipe();
	void	setupChild();

public:
// Lifecycle
	CgiHandler(HttpRequest& request, Client& client, std::vector<struct pollfd>& pfds);
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
};


#endif