#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <rendering/window.hpp>

const int MAX_FRAMES_IN_FLIGHT = 2;

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

	vk::Buffer createBuffer(vk::DeviceSize size, vk::BufferUsageFlags flags, vk::MemoryPropertyFlags properties, vk::DeviceMemory& bufferMemory);

	void tick(Window& window);
	void end();
private:
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapChain;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;
	vk::RenderPass renderPass;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorPool descriptorPool;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;
	vk::CommandPool commandPool;
	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexBufferMemory;
	vk::Buffer indexBuffer;
	vk::DeviceMemory indexBufferMemory;

	std::vector<vk::CommandBuffer> commandBuffers;
	std::vector<vk::Image> swapChainImages;
	std::vector<vk::ImageView> swapChainImageViews;
	std::vector<vk::Framebuffer> swapChainFramebuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;

	std::vector<vk::Buffer> uniformBuffers;
	std::vector<vk::DeviceMemory> uniformBuffersMemory;
	std::vector<uint8_t *> uniformBuffersMapped;

	std::vector<vk::DescriptorSet> descriptorSets;

	std::vector<const char *> extensions;
	const std::vector<const char *> deviceExtensions = {
		"VK_KHR_swapchain"
	};
	const std::vector<const char *> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	uint32_t currentFrame = 0;

	void createInstance();
	void pickPhysicalDevice();
	bool checkLayers();
	void createDevice();
	void createSwapChain(Window& window);
	void createImageViews();
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();
	void createGraphicsPipeline();
	void createRenderPass();
	void createFramebuffers();
	void createCommandPool();
	void createIndexBuffer();
	void createVertexBuffer();
	void createUniformBuffers();
	void createCommandBuffers();
	void createSyncObjects();
	void recreateSwapChain(Window& window);
	void cleanupSwapChain();


	void recordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex);
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	void updateUniformBuffer(uint32_t currentImage);

	QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, Window& window);

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
};
