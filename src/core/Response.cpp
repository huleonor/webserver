#include "../../include/Response.hpp"
#include "../../include/ServerConfig.hpp"
#include <sstream>
#include <fstream>

/* ----------------------- Constructor and Destructor ----------------------- */
Response::Response() : _status_code(0) {}
Response::~Response() {}

/* --------------------------------- Setter --------------------------------- */
void	Response::setCodeStatus(int code)	{ _status_code = code; }
void	Response::setStatusPhrase(const std::string& phrase)	{ _status_phrase = phrase; }
void	Response::setFullResponse()	{ _full_response = _first_line + _headers + _body; }

void	Response::setFirstLine()
{
	std::ostringstream	oss;

	oss << "HTTP/1.1 " << _status_code << " " << _status_phrase << "\r\n";
	_first_line = oss.str();
}

void	Response::setErrorBody(const ServerConfig& server)
{
	const std::map<int, std::string>& error_pages = server.getErrorPages();
	std::map<int, std::string>::const_iterator it = error_pages.find(_status_code);
	if (it == error_pages.end())
	{
		generateDefaultErrorPage();
		return ;
	}
	std::string		full_path = server.getRoot() + it->second;
	std::ifstream	file(full_path.c_str());
	if (!file.is_open())
	{
		generateDefaultErrorPage();
		return ;
	}
	std::string s;
	while (std::getline(file, s))
		_body += s += '\n';
}
void	Response::setHeaders()
{
	std::ostringstream	oss;

	oss << "Content-Type: text/html\r\n" << "Content-Length: " << _body.size() << "\r\n" << "\r\n";
	_headers = oss.str();
}

/* --------------------------------- Getters --------------------------------- */
const std::string&	Response::getFirstLine() const		{ return (_first_line); }
const std::string&	Response::getHeaders() const		{ return (_headers); }
const std::string&	Response::getBody() const			{ return (_body); }
const std::string&	Response::getFullResponse() const	{ return (_full_response); }

/* ----------------------------- Build responses ---------------------------- */
void	Response::buildError(const ServerConfig& server)
{
	setFirstLine();
	setErrorBody(server);
	setHeaders();
	setFullResponse();
}

/* ----------------------- Generate default error page ---------------------- */
void	Response::generateDefaultErrorPage()
{
	std::ostringstream	oss;

	oss << "<html><body><h1>" <<  _status_code << " " << _status_phrase << "</h1></body></html>";
	_body = oss.str();
}