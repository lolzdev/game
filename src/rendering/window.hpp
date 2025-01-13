#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

class Window {
public:
	Window(std::string title, uint32_t width, uint32_t height);
	~Window();

	bool framebufferResized;

	vk::Extent2D framebufferSize();
	bool shouldClose();
	void tick();
	vk::SurfaceKHR createSurface(vk::Instance instance);
private:
	GLFWwindow *raw = nullptr;
};
