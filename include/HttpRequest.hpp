#ifndef HPPREQUEST_HPP
# define HPPREQUEST_HPP

#include <string>
#include <map>

class HppRequest
{
	private:
		std::string	_method;
		std::string	_path;
		std::string	_query_string;
		std::string _version;
		std::map<std::string, std::string>	_headers;

	public:
	// Constructor and destructor
		HppRequest();
		~HppRequest();
	// Setters
		void	setMethod(const std::string& method);
		void	setPath(const std::string& path);
		void	setQueryString(const std::string& query);
		void	setVersion(const std::string& version);
		void	setHeaders(const std::string& key, const std::string& value);

	// Getters
		const std::string&	getMethod() const;
		const std::string&	getPath() const;
		const std::string&	getQueryString() const;
		const std::string&	getVersion() const;
		const std::map<std::string, std::string>&	getHeaders() const;

	
};

#endif