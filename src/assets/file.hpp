#pragma once

#include <assets/assets.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

std::filesystem::path getFilePath(Identifier id, AssetType ty);
std::vector<char> readFile(Identifier id, AssetType ty);
