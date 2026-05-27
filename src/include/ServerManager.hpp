#ifndef SERVERMANAGER_HPP
# define SERVERMANAGER_HPP

#include <vector>
#include "ServerConfig.hpp"

class ServerManager
{
	private:
		std::vector<ServerConfig>	_servers;

	public:
		ServerManager();
		~ServerManager();

		void	setupServers();
};

#endif