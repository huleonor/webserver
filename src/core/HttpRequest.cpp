#include "../../include/HttpRequest.hpp"
#include "../../include/utils.hpp"
#include <sstream>
#include <cctype>
#include <iostream>
#include <cerrno>

HttpRequest::HttpRequest() : error_code(0) {}
HttpRequest::~HttpRequest() {}

void	HttpRequest::parseMultipart(const std::string& boundary)
{
	std::string	delimiter = "--" + boundary;
	std::string	final_delimiter = delimiter + "--";
	size_t		pos = body.find(delimiter);

	while (pos != std::string::npos)
	{
		// skip past the delimiter line
		size_t part_start = body.find("\r\n", pos);
		if (part_start == std::string::npos)
			return ;
		part_start += 2;

		// check if it's the final boundary
		if (body.substr(pos, final_delimiter.size()) == final_delimiter)
			return ;

		// find end of part headers
		size_t headers_end = body.find("\r\n\r\n", part_start);
		if (headers_end == std::string::npos)
			return ;

		// extract part headers block
		std::string	part_headers = body.substr(part_start, headers_end - part_start);

		// extract content — between \r\n\r\n and next boundary
		size_t content_start = headers_end + 4;
		size_t next_boundary = body.find("\r\n" + delimiter, content_start);
		if (next_boundary == std::string::npos)
			return ;

		UploadFile	file;
		size_t fn_pos = part_headers.find("filename=\"");
		if (fn_pos != std::string::npos)
		{
			fn_pos += 10;
			size_t fn_end = part_headers.find("\"", fn_pos);
			if (fn_end != std::string::npos)
				file.filename = part_headers.substr(fn_pos, fn_end - fn_pos);
		}
		file.content = body.substr(content_start, next_boundary - content_start);
		if (!file.filename.empty() && !file.content.empty() && file.filename.find("..") == std::string::npos)
			uploads.push_back(file);
		pos = body.find(delimiter, next_boundary);
	}
}

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

	line_request = line;
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
// Check path accessibility and populate target_info
bool	HttpRequest::isValidPath()
{
	if (stat(path.c_str(), &target_info) == -1)
	{
		if (errno == EACCES)
			error_code = 403;
		else if (errno == ENOENT)
			error_code = 404;
		else
			error_code = 500;
	}
	return (error_code);
}

// resolves ".." segments and returns false if the path tries to escape the root
bool	HttpRequest::resolvePathWithinRoot()
{ 
	std::stringstream	ss(path);
	std::vector<std::string>	segments;
	std::vector<std::string>	resolved;
	std::string	token;
	while (std::getline(ss, token, '/'))
		segments.push_back(token);
	for (size_t i = 0; i < segments.size(); i++)
	{
		if (segments[i] != "..")
			resolved.push_back(segments[i]);
		else
		{
			resolved.pop_back();
			if (resolved.empty())
				break;
		}	
	}
	path.clear();
	for (size_t i = 0; i < resolved.size(); i++)
		path+= (resolved[i] + '/');
	normalizeSlash(path);
	return (!resolved.empty()); 
}
