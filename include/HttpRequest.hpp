#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

#include <string>
#include <map>

struct HttpRequest
{
	std::string							method;
	std::string							path;
	std::string							query_string;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
	int									error_code;

	HttpRequest();
	~HttpRequest();
	void	parse(const std::string& header);
};

#endif
