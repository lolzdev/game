#pragma once

#include <ostream>

enum AssetType {
	Shader,
	Texture
};

class Identifier {
public:
	Identifier(std::string space, std::string name);
	std::string space;
	std::string name;
};
