#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include "ServerConfig.hpp"
#include "Client.hpp"
#include <vector>
#include <map>
#include <poll.h>

class ServerManager
{
	private:
	// Attributes
		std::map<int, Client>	_clients;
		std::vector<struct pollfd> _pfds;
		std::vector<ServerConfig> _servers;
		static	bool	_running;
	// Client Management
		void	acceptNewClient(size_t pfds_pos);
		void	handleClientRequest(size_t& pfds_pos);
		void	handleClientResponse(size_t& pfds_pos);
		void	closeConnection(size_t& pfds_pos, const std::string& msg);
	// Error handling
		void	handleInitOrAcceptError(int fd, const std::string& msg);
	// Runtime
		void	handleEvent();
		bool	isServerSocket(size_t pos);

	public:
	// Iterator
		typedef std::map<int, Client>::iterator	client_it;
	// Getters
		std::vector<ServerConfig>  &getServers();
		size_t	size();
	// Setup
		void	setupServers();
		void	addServer(const ServerConfig &server);
	//Runtime
		void	start();
	// Signal handling
		static void	handleSignal(int sig);
		
	// Temp
		void	print(); /// tmp function
};

#endif
