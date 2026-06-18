#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include "ServerConfig.hpp"
#include "Client.hpp"
#include <vector>
#include <map>
#include <poll.h>

class ServerManager
{
public:
// Types
	typedef std::map<int, Client*>::iterator	client_it;
private:
// Attributes
	std::map<int, Client*>		_clients;
	std::map<int, Client*>		_cgi_pipes;
	std::vector<struct pollfd>	_pfds;
	std::vector<ServerConfig>	_servers;
	static bool					_running;
// Non-copyable (single runtime manager)
	ServerManager(const ServerManager&);
	ServerManager& operator=(const ServerManager&);
// Client Management
	void	acceptNewClient(size_t pfds_pos);
	void	handleClientRequest(size_t& pfds_pos);
	void	handleClientResponse(size_t& pfds_pos);
	void	processClientRequest(Client& client);
	void	closeConnection(size_t& pfds_pos);
// Error handling
	void	handleInitOrAcceptError(int fd, const std::string& msg);
// Runtime
	void	handleClientPollCleanUp(size_t& pfds_pos);
	void	handleCgiPollCleanUp(size_t& pfds_pos);
	void	handleEvent();
	bool	isServerSocket(size_t pos);
	void	monitorClients();

public:
// Lifecycle
	ServerManager();
	~ServerManager();
// Getters
	std::vector<ServerConfig>&	getServers();
	size_t						size();
// Setup
	void	addServer(const ServerConfig& server);
	void	setupServers();
// Runtime
	void	start();
// Signal handling
	static void	handleSignal(int sig);
};

#endif
