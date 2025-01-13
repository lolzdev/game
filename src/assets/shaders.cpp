#include <assets/shaders.hpp>
#include <assets/file.hpp>

std::vector<uint32_t> compileShader(Identifier id, ShaderType ty) {
	shaderc_shader_kind kind;

	switch (ty) {
		case ShaderType::Vertex:
			kind = shaderc_vertex_shader;
			break;
		case ShaderType::Fragment:
			kind = shaderc_fragment_shader;
			break;
	}

	shaderc::Compiler compiler;
	std::vector<char> source = readFile(id, AssetType::Shader);
	std::string code(source.begin(), source.end());

	auto result = compiler.CompileGlslToSpv(code, kind, id.name.c_str());

	std::vector<uint32_t> bytecode(result.begin(), result.end());

	return bytecode;
}

vk::ShaderModule createShaderModule(Identifier id, ShaderType ty, vk::Device device) {
	std::vector<uint32_t> code = compileShader(id, ty);
	vk::ShaderModuleCreateInfo moduleCreateInfo(vk::ShaderModuleCreateFlags(), code);
	return device.createShaderModule(moduleCreateInfo);
}
