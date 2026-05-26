#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>

class Location
{
private:
	std::string _path;
	std::string _root;

public:
	Location();
	~Location();

	std::string getPath() const;
	std::string getRoot() const;

	void setPath(const std::string &path);
	void setRoot(const std::string &root);
};

#endif
