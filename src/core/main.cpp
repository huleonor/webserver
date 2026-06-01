#include "../../include/ConfigParser.hpp"
#include "../../include/ServerManager.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

int main(int argc, char **argv)
{
	if (argc == 1 || argc == 2)
	{
		try
		{
			ServerManager manager;
			ConfigParser parser;
			parser.parse(argc, argv, manager);
			manager.print();
      		manager.setupServers();
			manager.runServer();
		}
		catch (std::exception &e)
		{
			std::cerr << "\033[31mError: " << e.what() << "\033[0m\n";
			return 1;
		}
	}
	else
	{
		std::cout << "Error: wrong arguments" << std::endl;
		return 1;
	}
	return 0;
}