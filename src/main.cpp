#include "include/ConfigParser.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    ConfigParser parser;
	if (argc == 1 || argc == 2)
    {
        try
        {
            parser.parse(argc, argv);
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