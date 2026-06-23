#ifndef CGI_HANDLER
# define CGI_HANDLER

#include "HttpRequest.hpp"
#include <unistd.h>
#include <poll.h>
#include <string>
#include <vector>
#include <ctime>

class Client;

class CgiHandler
{
public:
	enum Status
	{
		NONE,
		DONE,
		ERROR
	};
private:
// --- Attributes ---
	Status						_status;
	int							_pipe_body[2];
	int							_pipe_output[2];
	pid_t						_pid;
	size_t						_body_bytes_sent;
	time_t						_start_time;
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

// --- Non-copyable ---
	CgiHandler(const CgiHandler& other);
	CgiHandler&	operator=(const CgiHandler& other);

// --- Internal: Setup ---
	void	setEnv();
	void	setupPipe();
	void	setupChild();
	void	closeAllFds();

public:
// --- Lifecycle ---
	CgiHandler(HttpRequest& request, Client& client);
	~CgiHandler();

// --- Getters ---
	const std::string&	getExt() const;
	const std::string&	getFilename() const;
	const std::string&	getScriptName() const;
	const std::string&	getPathInfo() const;
	const std::string&	getCgiOutputBuffer() const;
	pid_t				getPid() const;
	time_t				getStartTime() const;
	const int*			getPipeBody() const;
	const int*			getPipeOutput() const;
	Status				getStatus() const;

// --- Setters ---
	void				setStatus(Status status);
	void				setInterpreterPath(const std::string& path);

// --- CGI Parsing ---
	void	extractCgiInfo(const std::string& loc);

// --- CGI Setup ---
	struct pollfd	cgiSetup();

// --- CGI Process ---
	void	receiveCgiOutput(const std::string& buffer);
	void	sendBody(const std::string& body);
	int		checkWaitpid();
	void	closeAllOpenPipeFds();
	void	closePipeFd(int fd);
};


#endif