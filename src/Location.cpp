#include "include/Location.hpp"

Location::Location() {}
Location::~Location() {}

std::string Location::getPath() const { return _path; }
std::string Location::getRoot() const { return _root; }

void Location::setPath(const std::string &path) { _path = path; }
void Location::setRoot(const std::string &root) { _root = root; }
