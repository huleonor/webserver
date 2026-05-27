#include "include/ConfigParser.hpp"
#include "include/ServerManager.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

int main(int argc, char **argv)
{
	if (argc == 1 || argc == 2)
    {
        try
        {
			ConfigParser parser;
			ServerManager manager;
            parser.parse(argc, argv);
			manager.setupServers();
        }
        catch (std::exception &e)
        {
            std::cerr << "\033[31mError: " <<  e.what() << "\033[0m\n";
        }
    }
	else
	{
		std::cout << "Error: wrong arguments" << std::endl;
		return 1;
	}
	return 0;
}