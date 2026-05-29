#include "include/Location.hpp"

Location::Location() : _autoindex(false) {}
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
