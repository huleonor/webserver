#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <netinet/in.h>
#include <string>
#include <map>
#include "ServerConfig.hpp"

class Client
{
	public:
		enum Status
		{
			READING_HEADER,
			READING_BODY,
			WRITING
		};
	private:
		struct HttpRequest 
		{
			std::string	method;
			std::string	path;
			std::string	query_string;
			std::string version; 
			std::map<std::string, std::string>	headers;
			bool	headers_parsed;
			bool	body_parsed;
    	};
	// Attribues
		int				_client_socket;
		std::string		_request_buffer;
		std::string		_response_buffer;
		HttpRequest 	_request;
		ServerConfig&	_server;
		in_addr_t		_client_addr;
		in_port_t		_client_port;
		Status			_status;

	public:
	// Constants
		static const int	MAX_HEADER_SIZE = 8192;
	// Constructor and Destructor
		Client(ServerConfig& server, int fd, struct sockaddr_in& addr);
		~Client();
	// Getters
		in_port_t	getClientPort() const;
		in_addr_t	getClientAddr() const;
		Status		getStatus() const;
	// Request Handling
		void	receiveHeader(const std::string& request);
		void	receiveBody(const std::string& request);
};

#endif
