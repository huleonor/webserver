#include "../../include/HttpRequest.hpp"
#include <sstream>
#include <cctype>

HttpRequest::HttpRequest() : error_code(0) {}

HttpRequest::HttpRequest(const HttpRequest& other)
    : method(other.method),
      path(other.path),
      query_string(other.query_string),
      version(other.version),
      headers(other.headers),
      body(other.body),
      error_code(other.error_code) {}

HttpRequest& HttpRequest::operator=(const HttpRequest& other)
{
	if (this != &other)
	{
		method = other.method;
		path = other.path;
		query_string = other.query_string;
		version = other.version;
		headers = other.headers;
		body = other.body;
		error_code = other.error_code;
	}
	return *this;
}

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

	// Parse request line: METHOD URI VERSION
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
	if (uri.empty() || uri[0] != '/')
	{
		error_code = 400;
		return ;
	}

	// Split URI into path and query_string
	size_t qpos = uri.find('?');
	if (qpos != std::string::npos)
	{
		path = uri.substr(0, qpos);
		query_string = uri.substr(qpos + 1);
	}
	else
		path = uri;

	// Parse headers
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
