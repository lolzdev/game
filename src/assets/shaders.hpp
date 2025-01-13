#pragma once

#include <vector>
#include <shaderc/shaderc.hpp>
#include <assets/assets.hpp>
#include <vulkan/vulkan.hpp>

enum ShaderType {
	Vertex,
	Fragment
};

vk::ShaderModule createShaderModule(Identifier id, ShaderType ty, vk::Device device);
std::vector<uint32_t> compileShader(Identifier id, ShaderType ty);
