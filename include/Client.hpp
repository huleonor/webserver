#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <netinet/in.h>
#include <string>
#include <map>
#include <ctime>
#include "ServerConfig.hpp"
#include "Response.hpp"
#include "HttpRequest.hpp"

// Not copyable: each instance owns a unique TCP socket fd that must not be duplicated.
class Client
{
public:
	enum Status
	{
		READING_HEADER,
		READING_BODY,
		PROCESSING,
		WRITING,
		ERROR,
		CLOSE
	};
	struct CgiInfo
	{
		pid_t	cgi_pid; 
		int		client_cgi_fd; // read end of CGI output pipe
		time_t	start_cgi;
	};
	

private:
// Attributes
	int				_client_socket;
	in_addr_t		_client_addr;
	in_port_t		_client_port;
	ServerConfig*	_server;
	std::string		_request_buffer;
	HttpRequest		_request;
	size_t			_content_length;
	bool			_chunked;
	Status			_status;
	Response		_response;
	ssize_t			_bytes_sent;
	time_t			_last_time_activity;
	CgiInfo*		_cgi_info;
// Non-copyable (owns a unique socket fd)
	Client(const Client& other);
	Client&	operator=(const Client& other);
// Request Handling
	void				receiveHeader(const std::string& request);
	void				receiveBody(const std::string& request);
	bool				hasCompleteBody();
	void				parseMultipartIfNeeded();
	// Response
	void				buildAutoindex(const std::string& dirPath);
	void				buildUploadFromPath(const Location& loc);
	void				postContent();
	void				executeCGI();

public:
// Constants
	static const int	MAX_HEADER_SIZE = 8192;
// Lifecycle
	Client(int fd, struct sockaddr_in& addr, ServerConfig& server);
	~Client();
// Getters
	int					getClientSocket() const;
	CgiInfo*			getClientCgi() const;
	in_port_t			getClientPort() const;
	in_addr_t			getClientAddr() const;
	Status				getStatus() const;
	const HttpRequest&	getRequest() const;
	const Response&		getResponse() const;
	const std::string&	getFullResponse() const;
	const ServerConfig*	getClientServer() const;
	ssize_t				getBytesSent() const;
	time_t				getLastTimeActivity() const;
	const std::string&	getLogMsg() const;
// Setters
	void				setStatus(Status status);
	void				setBytesSent(ssize_t n);
	void				setLastTimeActivity(time_t time);
	void				setLogMsg(const std::string& msg);
// Response
	void				buildErrorResponse(int code);
	void				handleGet(const Location& loc);
	void				handlePost(const Location& loc);
	void				handleCGI(const Location& loc);
	void				handleDelete();
	void				sendResponse();
	void				validateAndReplacePath(const Location& loc);
// Request Handling
	ssize_t				receiveData();
};

#endif
