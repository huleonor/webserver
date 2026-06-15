#include "../../include/utils.hpp"
#include <map>

std::string	getErrorPhrase(int code)
{
	static std::map<int, std::string> phrases;
	if (phrases.empty())
	{
		phrases[400] = "Bad Request";
		phrases[403] = "Forbidden";
		phrases[404] = "Not Found";
		phrases[405] = "Method Not Allowed";
		phrases[408] = "Request Timeout";
		phrases[411] = "Length Required";
		phrases[413] = "Content Too Large";
		phrases[500] = "Internal Server Error";
		phrases[501] = "Not Implemented";
		phrases[501] = "Gateway Timeout";
		phrases[505] = "HTTP Version Not Supported";
	}
	std::map<int, std::string>::iterator it = phrases.find(code);
	if (it != phrases.end())
		return it->second;
	return "Unknown Error";
}

void	normalizeSlash(std::string& str)
{
	if (!str.empty() && str[0] == '/')
		str.erase(0, 1);
	if (!str.empty() && str[str.size() - 1] == '/')
		str.erase(str.size() - 1);
}
