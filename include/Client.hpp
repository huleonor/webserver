#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <netinet/in.h>
#include <string>
#include <map>
#include "ServerConfig.hpp"

class Client
{
	private:
	// DataTypes
		struct HttpRequest 
		{
			std::string	_method;
			std::string	_path;
			std::string	_query_string;
			std::string _version; 
			std::map<std::string, std::string>	_headers;
			bool	headers_parsed;
			bool	body_parsed;  
    	};
		enum Staus
		{
			READING_HEADER,
			READING_BODY,
			WRITING
		};

	// Attribues
		int				_client_socket;
		std::string		_request_buffer;
		std::string		_response_buffer;
		HttpRequest 	_request;
		ServerConfig&	_server;
		in_addr_t		_client_addr;
		in_port_t		_client_port;

	public:
	// Constructor and Destructor
		Client(ServerConfig& server, int fd, struct sockaddr_in& addr);
		~Client();

		in_port_t	getClientPort() const;
		in_addr_t	getClientAddr() const;
};

#endif
