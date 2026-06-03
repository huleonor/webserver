#include "../../include/HttpRequest.hpp"

/* ----------------------- Constructor and Destructor ----------------------- */
HppRequest::HppRequest() {}
HppRequest::~HppRequest() {}

/* --------------------------------- Setters -------------------------------- */
void	HppRequest::setMethod(const std::string& method) { _method = method; }
void	HppRequest::setPath(const std::string& path) { _path = path; }
void	HppRequest::setQueryString(const std::string& query) { _query_string = query; }
void	HppRequest::setVersion(const std::string& version) { _version = version; }
void	HppRequest::setHeaders(const std::string& key, const std::string& value) { _headers[key] = value; }

/* --------------------------------- Getters -------------------------------- */
const std::string&	HppRequest::getMethod() const { return (_method); }
const std::string&	HppRequest::getPath() const { return (_path); }
const std::string&	HppRequest::getQueryString() const { return (_query_string); }
const std::string&	HppRequest::getVersion() const { return (_version); }
const std::map<std::string, std::string>&	HppRequest::getHeaders() const { return (_headers); }
