#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>

struct UploadFile
{
	std::string	filename;
	std::string	content;
};

struct HttpRequest
{
	std::string							method;
	std::string							path;
	std::string							query_string;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
	int									error_code;
	std::vector<UploadFile>				uploads;

	HttpRequest();
	~HttpRequest();
	void	parse(const std::string& header);
	bool	isSafeUri(const std::string uri) const;
	void	parseMultipart(const std::string& boundary);
};

#endif
