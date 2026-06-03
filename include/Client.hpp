#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <netinet/in.h>
#include <string>
#include <map>
#include "ServerConfig.hpp"
#include "Response.hpp"
#include "HttpRequest.hpp"

class Client
{
	public:
		enum Status
		{
			READING_HEADER,
			READING_BODY,
			WRITING,
			ERROR
		};
	private:
	// Attribues
		int				_client_socket;
		in_addr_t		_client_addr;
		in_port_t		_client_port;
		ServerConfig&	_server;
		std::string		_request_buffer;
		HppRequest 		_request;
		Status			_status;
		Response		_response;
		ssize_t			_bytes_sent;

	public:
	// Constants
		static const int	MAX_HEADER_SIZE = 8192;
	// Constructor and Destructor
		Client(int fd, struct sockaddr_in& addr, ServerConfig& server);
		~Client();
	// Getters
		int					getClientSocket() const;
		in_port_t			getClientPort() const;
		in_addr_t			getClientAddr() const;
		Status				getStatus() const;
		const std::string&	getResponse() const;
		ssize_t				getBytesSent() const;
	// Setters
		void		setStatus(Status status);
		void		setBytesSent(int n);
	// Response
		void	buildErrorResponse();
	// Request Handling
		void	receiveHeader(const std::string& request);
		void	receiveBody(const std::string& request);
};

#endif
