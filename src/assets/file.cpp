#include <assets/file.hpp>
namespace fs = std::filesystem;

fs::path getFilePath(Identifier id, AssetType ty) {
	std::string assetDirectory;
	std::string assetExtension;

	switch (ty) {
		case AssetType::Shader:
			assetDirectory = "shaders";
			assetExtension = ".glsl";
			break;
		case AssetType::Texture:
			assetDirectory = "textures";
			assetExtension = ".png";
			break;
	}

	return fs::current_path() / ".." / "mods" / id.space / "assets" / assetDirectory / (id.name + assetExtension);
}

std::vector<char> readFile(Identifier id, AssetType ty) {
	auto filename = getFilePath(id, ty);
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}
