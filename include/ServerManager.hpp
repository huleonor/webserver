#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include "ServerConfig.hpp"
#include "Client.hpp"
#include <poll.h>
#include <vector>
#include <map>

class ServerManager
{
public:
// --- Types ---
	typedef std::map<int, Client*>::iterator	client_it;

private:
// --- Attributes ---
	std::map<int, Client*>		_clients;
	std::map<int, Client*>		_cgi_pipes;
	std::vector<struct pollfd>	_pfds;
	std::vector<ServerConfig>	_servers;
	static bool					_running;

// --- Non-copyable ---
	ServerManager(const ServerManager&);
	ServerManager& operator=(const ServerManager&);

// --- Internal: Client ---
	void	acceptNewClient(size_t pfds_pos);
	void	handleClientRequest(size_t& pfds_pos);
	void	handleClientResponse(size_t& pfds_pos);
	void	processClientRequest(Client& client);
	void	closeConnection(size_t& pfds_pos);
	void	handleClientPollCleanUp(size_t& pfds_pos);

// --- Internal: CGI ---
	void	handleCgiProcess(size_t& pfds_pos);
	void	sendCgiBody(size_t& pfds_pos);
	void	processCgiOutput(Client* client, CgiHandler* cgi, size_t& pfds_pos);
	void	closeFdAndCleanMaps(CgiHandler* cgi, size_t& pfds_pos, bool closeFds);
	void	processCgiClientResponse(Client* client, int code, const std::string body, const std::string content_type = "text/html");
	void	handleCgiPollCleanUp(size_t& pfds_pos, const std::string& logMsg);

// --- Internal: Runtime ---
	void	handleEvent();
	void	monitorClients();
	bool	isServerSocket(size_t pos);

// --- Internal: Error ---
	void	handleInitOrAcceptError(int fd, const std::string& msg);

public:
// --- Lifecycle ---
	ServerManager();
	~ServerManager();

// --- Getters ---
	std::vector<ServerConfig>&	getServers();
	size_t						size();

// --- Setup ---
	void	addServer(const ServerConfig& server);
	void	setupServers();

// --- Runtime ---
	void	start();

// --- Signal ---
	static void	handleSignal(int sig);
};

#endif
