#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <poll.h>
#include <vector>
#include <map>
#include "ServerConfig.hpp"
#include "Client.hpp"

class ServerManager
{
	private:
	// Attributes
		std::vector<ServerConfig> _servers;
		std::map<int, Client>	_clients;
		std::vector<struct pollfd> _pfds;

	// Private methods
		void	handleEvent();
		void 	handleNewClient(int pfds_pos);

	public:
	// Constructor and Destructor
		ServerManager();
		~ServerManager();

	// Getters
		std::vector<ServerConfig>  &getServers();
	
	// Methods
		size_t                     size() const;
		void                       addServer(const ServerConfig &server);
		void					   runServer();
		void	                   setupServers();
		void                       print() const; /// tmp function
};

#endif
