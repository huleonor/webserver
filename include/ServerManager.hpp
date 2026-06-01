#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include "ServerConfig.hpp"
#include "Client.hpp"
#include <vector>
#include <map>

class ServerManager
{
	private:
	// Attributes
		std::vector<ServerConfig> _servers;
		std::map<int, Client>	_clients;
		std::vector<struct pollfd> _pfds;
		static bool	_running;

	// Private methods
		void	handleEvent();
		void 	acceptNewClient(int pfds_pos);
		void	handleInitOrAcceptError(int fd, const std::string& msg);
		void	handleClientError(int pfds_pos, const std::string& msg);

	public:
		typedef std::map<int, Client>::iterator	client_it;

	// Constructor and Destructor
		ServerManager();
		~ServerManager();

	// Getters
		std::vector<ServerConfig>  &getServers();
	
	// Methods
		size_t	size() const;
		void	addServer(const ServerConfig &server);
		void	handleClientRequest(size_t& pfds_pos);
		void	setupServers();
		void	start();
		void	print() const; /// tmp function
	
	// Public Static Methods
		static void		handleSignal(int sig);
};

#endif
