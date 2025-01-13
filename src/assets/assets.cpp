#include <assets/assets.hpp>

Identifier::Identifier(std::string space, std::string name) : space(space), name(name) {}

std::ostream& operator<<(std::ostream &out, Identifier const& data) {
    out << data.space << ':' << data.name;
    return out;
};
