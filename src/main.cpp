#include "include/ConfigParser.hpp"
#include <iostream>

int main(int argc, char **argv)
{
	std::string config_file;
    ConfigParser parser;
	if (argc == 1 || argc == 2)
    {
        try
        {
            config_file = (argc == 1 ? "configs/default.conf" : argv[1]);
		    std::string content = parser.readFile(config_file);
		    std::cout << "File content:\n" << content << std::endl;
        }
        catch (std::exception &e)
        {
            std::cerr << e.what();
        }
        
    }
	else
	{
		std::cout << "Error: wrong arguments" << std::endl;
		return 1;
	}
	return 0;
}