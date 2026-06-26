#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>

class Location
{
private:
// --- Attributes ---
	std::string              _path;
	std::string              _root;
	std::string              _index;
	bool                     _autoindex;
	std::vector<std::string> _allow_methods;
	std::string              _return;
	std::vector<std::string> _cgi_ext;
	std::vector<std::string> _cgi_path;
	std::string              _upload_path;
	unsigned long            _client_max_body_size;

public:
// --- Lifecycle ---
	Location();
	Location(const Location& other);
	Location&	operator=(const Location& other);
	~Location();

// --- Getters ---
	const std::string&              getPath() const;
	const std::string&              getRoot() const;
	const std::string&              getIndex() const;
	bool                            getAutoindex() const;
	const std::vector<std::string>& getAllowMethods() const;
	const std::string&              getReturn() const;
	const std::vector<std::string>& getCgiExt() const;
	const std::vector<std::string>& getCgiPath() const;
	const std::string&              getUploadPath() const;
	unsigned long                   getClientMaxBodySize() const;

// --- Setters ---
	void	setPath(const std::string& path);
	void	setRoot(const std::string& root);
	void	setIndex(const std::string& index);
	void	setAutoindex(bool autoindex);
	void	addAllowMethod(const std::string& method);
	void	setReturn(const std::string& redirect);
	void	addCgiExt(const std::string& ext);
	void	addCgiPath(const std::string& path);
	void	setUploadPath(const std::string& path);
	void	setClientMaxBodySize(unsigned long size);

// --- Validations ---
	bool	isValidMethod(const std::string& method) const;
	bool	findFileExtension(const std::string& path) const;
};

#endif
