#pragma once

#include <ostream>

class Identifier {
public:
	Identifier(std::string space, std::string name);
	std::string space;
	std::string name;
};

std::ostream& operator<<(std::ostream &out, Identifier const& data) {
    out << data.space << ':' << data.name;
    return out;
};
