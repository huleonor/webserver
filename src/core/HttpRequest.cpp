#include "../../include/HttpRequest.hpp"
#include <sstream>
#include <cctype>

HttpRequest::HttpRequest() : error_code(0) {}
HttpRequest::~HttpRequest() {}

static void	trimLeft(std::string& str)
{
	size_t start = str.find_first_not_of(" \t");
	if (start != std::string::npos)
		str = str.substr(start);
	else
		str.clear();
}

static void	toLower(std::string& str)
{
	for (size_t i = 0; i < str.size(); i++)
		str[i] = std::tolower(str[i]);
}

void	HttpRequest::parse(const std::string& header)
{
	std::istringstream	stream(header);
	std::string			line;

	if (!std::getline(stream, line))
	{
		error_code = 400;
		return ;
	}
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	size_t pos1 = line.find(' ');
	size_t pos2 = line.rfind(' ');
	if (pos1 == std::string::npos || pos1 == pos2)
	{
		error_code = 400;
		return ;
	}
	method = line.substr(0, pos1);
	std::string uri = line.substr(pos1 + 1, pos2 - pos1 - 1);
	version = line.substr(pos2 + 1);

	if (method != "GET" && method != "POST" && method != "DELETE")
	{
		error_code = 501;
		return ;
	}
	if (version != "HTTP/1.1" && version != "HTTP/1.0")
	{
		error_code = 400;
		return ;
	}
	if (uri.empty() || uri[0] != '/' || !isSafeUri(uri))
	{
		error_code = 400;
		return ;
	}

	size_t qpos = uri.find('?');
	if (qpos != std::string::npos)
	{
		path = uri.substr(0, qpos);
		query_string = uri.substr(qpos + 1);
	}
	else
		path = uri;

	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			continue ;
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue ;
		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		trimLeft(value);
		toLower(key);
		headers[key] = value;
	}
}

bool	HttpRequest::isSafeUri(const std::string uri) const	{ return (uri.find("..") == std::string::npos); }
