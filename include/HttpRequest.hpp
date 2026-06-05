#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

#include <string>
#include <map>

class HttpRequest
{
public:
	std::string							method;
	std::string							path;
	std::string							query_string;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
	int									error_code;

	HttpRequest();
	HttpRequest(const HttpRequest& other);
	HttpRequest& operator=(const HttpRequest& other);
	~HttpRequest();
	void	parse(const std::string& header);
};

#endif
