#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <rendering/window.hpp>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

	bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

	bool isAdequate() {
		return !formats.empty() && !presentModes.empty();
	}
};

class Renderer {
public:
	Renderer(Window& window);
	~Renderer();
	void tick();
private:
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	std::vector<const char *> extensions;
	const std::vector<const char *> deviceExtensions = {
		"VK_KHR_swapchain"
	};
	const std::vector<const char *> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	void createInstance();
	void pickPhysicalDevice();
	bool checkLayers();
	void createDevice();
	void createSwapChain(Window &window);
	void createImageViews();
	void createGraphicsPipeline();

	QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, Window& window);

};
