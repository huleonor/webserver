#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

#include <sys/stat.h>
#include <string>
#include <map>
#include <vector>

struct UploadFile
{
	std::string	filename;
	std::string	content;
};

struct CgiInfo
{
	std::string	ext;
	std::string	filename;
	std::string script_name;
	std::string	path_info;
};


struct HttpRequest
{
	std::string							method;
	std::string							path;
	std::string							query_string;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
	std::vector<UploadFile>				uploads;
	std::string							line_request;
	int									error_code;
	struct	stat						target_info; // struct to save info from stat()

	HttpRequest();
	~HttpRequest();
	void	parse(const std::string& header);
	bool	resolvePathWithinRoot(const std::string& root);
	void	parseMultipart(const std::string& boundary);
	bool	isValidPath();
};

#endif
