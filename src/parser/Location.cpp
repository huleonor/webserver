#include "../../include/Location.hpp"
#include <algorithm>

Location::Location() : _autoindex(false) {}

Location::Location(const Location& other)
	: _path(other._path),
	  _root(other._root),
	  _index(other._index),
	  _autoindex(other._autoindex),
	  _allow_methods(other._allow_methods),
	  _return(other._return),
	  _cgi_ext(other._cgi_ext),
	  _cgi_path(other._cgi_path),
	  _upload_path(other._upload_path) {}

Location& Location::operator=(const Location& other)
{
	if (this != &other)
	{
		_path = other._path;
		_root = other._root;
		_index = other._index;
		_autoindex = other._autoindex;
		_allow_methods = other._allow_methods;
		_return = other._return;
		_cgi_ext = other._cgi_ext;
		_cgi_path = other._cgi_path;
		_upload_path = other._upload_path;
	}
	return *this;
}

Location::~Location() {}

std::string              Location::getPath() const         { return _path; }
std::string              Location::getRoot() const         { return _root; }
std::string              Location::getIndex() const        { return _index; }
bool                     Location::getAutoindex() const    { return _autoindex; }
std::vector<std::string> Location::getAllowMethods() const { return _allow_methods; }
std::string              Location::getReturn() const       { return _return; }
std::vector<std::string> Location::getCgiExt() const      { return _cgi_ext; }
std::vector<std::string> Location::getCgiPath() const     { return _cgi_path; }
std::string              Location::getUploadPath() const   { return _upload_path; }

void Location::setPath(const std::string &path)       { _path = path; }
void Location::setRoot(const std::string &root)       { _root = root; }
void Location::setIndex(const std::string &index)     { _index = index; }
void Location::setAutoindex(bool autoindex)            { _autoindex = autoindex; }
void Location::addAllowMethod(const std::string &m)   { _allow_methods.push_back(m); }
void Location::setReturn(const std::string &redirect) { _return = redirect; }
void Location::addCgiExt(const std::string &ext)      { _cgi_ext.push_back(ext); }
void Location::addCgiPath(const std::string &path)    { _cgi_path.push_back(path); }
void Location::setUploadPath(const std::string &path) { _upload_path = path; }

/* ------------------------------- Validations ------------------------------ */
bool Location::isValidMethod(const std::string& method) const
{
	return (std::find(_allow_methods.begin(), _allow_methods.end(), method) != _allow_methods.end());
}
