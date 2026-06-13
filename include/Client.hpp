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
// Non-copyable (owns a unique socket fd)
	Client(const Client& other);
	Client&	operator=(const Client& other);
// Request Handling
	void				receiveHeader(const std::string& request);
	void				receiveBody(const std::string& request);
	bool				hasCompleteBody();
	void				parseMultipartIfNeeded();
	void				buildAutoindex(const std::string& dirPath);
	void				buildUploadFromPath(const Location& loc);
	void				buildDirPath(const Location& loc, std::string& dirPath);
	bool				isValidDirPath(const std::string& dirPath);
	void				postContent(const std::string& dirPath);

public:
// Constants
	static const int	MAX_HEADER_SIZE = 8192;
// Lifecycle
	Client(int fd, struct sockaddr_in& addr, ServerConfig& server);
	~Client();
// Getters
	int					getClientSocket() const;
	in_port_t			getClientPort() const;
	in_addr_t			getClientAddr() const;
	Status				getStatus() const;
	const HttpRequest&	getRequest() const;
	const std::string&	getResponse() const;
	const ServerConfig*	getClientServer() const;
	ssize_t				getBytesSent() const;
	time_t				getLastTimeActivity() const;
// Setters
	void				setStatus(Status status);
	void				setBytesSent(ssize_t n);
	void				setLastTimeActivity(time_t time);
// Response
	void				buildErrorResponse(int code, const std::string& phrase);
	void				handleGet(const Location& loc);
	void				handleDelete(const Location& loc);
	void				handleCGI(const Location& loc);
	void				sendResponse();
// Request Handling
	ssize_t				receiveData();
	void				handlePost(const Location& loc);
};

#endif
