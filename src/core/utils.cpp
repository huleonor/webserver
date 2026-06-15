#include "../../include/utils.hpp"

void	normalizeSlash(std::string& str)
{
	if (!str.empty() && str[str.size() - 1] == '/')
		str.erase(str.size() - 1);
}
