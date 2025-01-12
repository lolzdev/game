#include <rendering/window.hpp>

Window::Window(std::string title, uint32_t width, uint32_t height) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	raw = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

Window::~Window() {
	glfwDestroyWindow(raw);
	glfwTerminate();
}

bool Window::shouldClose() {
	return glfwWindowShouldClose(raw);
}

void Window::tick() {
	glfwPollEvents();
}

vk::SurfaceKHR Window::createSurface(vk::Instance instance) {
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(instance, raw, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("can't create window surface!");
	}

	return surface;
}

vk::Extent2D Window::framebufferSize() {
	int width, height;
	glfwGetFramebufferSize(raw, &width, &height);

	return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}
