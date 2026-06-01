#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <vector>
#include "Parser/ServerConfig.hpp"

class ServerManager
{
private:
	std::vector<ServerConfig> _servers;

public:
	ServerManager();
	~ServerManager();

	void                       addServer(const ServerConfig &server);
	std::vector<ServerConfig>  &getServers();
	size_t                     size() const;
	void                       print() const;
	void	                     setupServers();
};

#endif
