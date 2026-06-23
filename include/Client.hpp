#ifndef CLIENT_HPP
# define CLIENT_HPP

#include "HttpRequest.hpp"
#include "CgiHandler.hpp"
#include "Response.hpp"
#include "ServerConfig.hpp"
#include <netinet/in.h>
#include <string>
#include <ctime>

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

private:
// --- Attributes ---
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
	CgiHandler*		_cgi;

// --- Non-copyable ---
	Client(const Client& other);
	Client&	operator=(const Client& other);

// --- Internal: Request ---
	void				receiveHeader(const std::string& request);
	void				receiveBody(const std::string& request);
	bool				hasCompleteBody();
	void				parseMultipartIfNeeded();

// --- Internal: Response ---
	void				buildAutoindex(const std::string& dirPath, const std::string& loc);
	void				buildUploadFromPath(const Location& loc);
	void				postContent();

public:
// --- Constants ---
	static const int	MAX_HEADER_SIZE = 8192;
	static const int	MAX_URI_SIZE = 4096;

// --- Lifecycle ---
	Client(int fd, struct sockaddr_in& addr, ServerConfig& server);
	~Client();

// --- Getters ---
	int					getClientSocket() const;
	in_port_t			getClientPort() const;
	in_addr_t			getClientAddr() const;
	Status				getStatus() const;
	const HttpRequest&	getRequest() const;
	const Response&		getResponse() const;
	const std::string&	getFullResponse() const;
	const ServerConfig*	getClientServer() const;
	CgiHandler*			getCgi() const;
	ssize_t				getBytesSent() const;
	time_t				getLastTimeActivity() const;
	const std::string&	getLogMsg() const;

// --- Setters ---
	void				setStatus(Status status);
	void				setBytesSent(ssize_t n);
	void				setLastTimeActivity(time_t time);
	void				setLogMsg(const std::string& msg);

// --- Request Handling ---
	ssize_t				receiveData();

// --- Response ---
	void				buildErrorResponse(int code);
	void				buildCgiResponse(const std::string& body);
	void				sendResponse();

// --- HTTP Methods ---
	void				handleGet(const Location& loc);
	void				handlePost(const Location& loc);
	void				handleDelete();
	void				handleCGI(const Location& loc);
	void				validateAndReplacePath(const Location& loc);
};

#endif
