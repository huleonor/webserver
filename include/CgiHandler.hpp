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
	std::string					_cgi_output_buffer;
	std::string					_ext;
	std::string					_filename;
	std::string					_interpreter_path;
	std::string 				_script_path;
	std::string					_path_info;
	HttpRequest&				_request;
	Client&						_client;
	std::vector<std::string>	_envTmp;
	std::vector<const char*>	_env;

// Non-copyable (owns a unique socket fd)
	CgiHandler(const CgiHandler& other);
	CgiHandler&	operator=(const CgiHandler& other);
// CGI Setup (private methods)
	void	setEnv();
	void	setupPipe();
	void	setupChild(char** argv);
	void	closeFds();

public:
// Lifecycle
	CgiHandler(HttpRequest& request, Client& client);
	~CgiHandler();

// Getters
	const std::string&	getExt() const;
	const std::string&	getFilename() const;
	const std::string&	getScriptName() const;
	const std::string&	getPathInfo() const;
	const std::string&	getCgiOutputBuffer() const;
	pid_t				getPid() const;
	const int*			getPipeBody() const;
	const int*			getPipeOutput() const;

// Setters
	void	setInterpreterPath(const std::string& path);

// CGI Process
	void	receiveCgiOutput(const std::string& buffer);

// CGI Parsing
	void	extractCgiInfo(const std::string& loc);
	struct pollfd	cgiSetup();
};


#endif