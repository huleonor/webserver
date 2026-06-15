#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include <string>
#include <map>

class Response
{
private:
// Attribute
	int			_status_code;
	std::string	_status_phrase;
	std::string	_first_line;
	std::string	_headers;
	std::string	_body;
	std::string	_full_response;
	std::string _log_close_msg;
// Generate default error page
	void				generateDefaultErrorPage();
// Setters
	void				setFirstLine();
	void				setErrorBody(const ServerConfig& server);
	void				setHeaders();
	void				setFullResponse();
public:
// Lifecycle
	Response();
	Response(const Response& other);
	Response& operator=(const Response& other);
	~Response();
// Setters
	void				setCodeStatus(int code);
	void				setStatusPhrase(const std::string& phrase);
	void				setBody(const std::string& body);
	void				setLogMsg(const std::string& msg);
// Getters
	const std::string&	getFirstLine() const;
	const std::string&	getBody() const;
	const std::string&	getHeaders() const;
	const std::string&	getFullResponse() const;
	const std::string&	gettLogMsg() const;
// Build Response 
	void				buildSuccess(const std::string& mimeType);
	void				buildRedirect(const std::string& location);
	void				buildError(const ServerConfig& server);
};

#endif
