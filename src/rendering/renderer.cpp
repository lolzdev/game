#include <rendering/renderer.hpp>
#include <rendering/window.hpp>
#include <iostream>
#include <set>
#include <limits>
#include <algorithm>

Renderer::Renderer(Window& window) {
	createInstance();
	surface = window.createSurface(instance);
	pickPhysicalDevice();
	createDevice();
	createSwapChain(window);
	createImageViews();
}

Renderer::~Renderer() {
	for (auto view : swapChainImageViews) {
		device.destroyImageView(view);
	}

	device.destroySwapchainKHR(swapChain);
	device.destroy();
	instance.destroySurfaceKHR(surface);
	instance.destroy();
}

bool Renderer::checkLayers() {
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

	for (const char *layerName : validationLayers) {
		bool found = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			return false;
		}
	}

	return true;
}

void Renderer::createInstance() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char *> glfwExt(glfwExtensions, glfwExtensions + glfwExtensionCount);
	extensions.insert(extensions.begin(), glfwExt.begin(), glfwExt.end());

	if (!checkLayers()) {
		throw std::runtime_error("validation layers are not available!");
	}

	try {
		vk::InstanceCreateInfo instanceCreateInfo({}, nullptr, validationLayers, extensions);
		instance = vk::createInstance(instanceCreateInfo);
	} catch (vk::SystemError & err) {
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		exit(-1);
	} catch (std::exception & err) {
		std::cout << "std::exception: " << err.what() << std::endl;
		exit(-1);
	} catch (...) {
		std::cout << "unknown error" << std::endl;
		exit(-1);
	}
}

void Renderer::pickPhysicalDevice() {
	auto devices = instance.enumeratePhysicalDevices();
	bool found = false;

	for (auto device : devices) {
		auto props = device.getProperties();
		std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& ext : availableExtensions) {
			requiredExtensions.erase(ext.extensionName);
		}

		if (requiredExtensions.empty()) {
			if (findQueueFamilies(device).isComplete() && querySwapChainSupport(device).isAdequate()) {
				std::cout << "Using " << props.deviceName << std::endl;
				physicalDevice = device;
				found = true;
				break;
			}
		}
	}

	if (!found) {
		throw std::runtime_error("can't find a suitable physical device");
	}
}

QueueFamilyIndices Renderer::findQueueFamilies(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;
	auto queueFamilies = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		if (device.getSurfaceSupportKHR(i, surface)) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) break;

		i++;
	}

    return indices;
}

void Renderer::createDevice() {
	auto indices = findQueueFamilies(physicalDevice);


	std::vector<vk::DeviceQueueCreateInfo> createInfos;
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
	float queuePriority = 1.0f;

	for (uint32_t family : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo createInfo(vk::DeviceQueueCreateFlags(), family, 1, &queuePriority);
		createInfos.push_back(createInfo);
	}

	device = physicalDevice.createDevice(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), createInfos, {}, deviceExtensions));

	graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
	presentQueue = device.getQueue(indices.presentFamily.value(), 0);
}

SwapChainSupportDetails Renderer::querySwapChainSupport(vk::PhysicalDevice device) {
	SwapChainSupportDetails details;
	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
	details.formats = device.getSurfaceFormatsKHR(surface);
	details.presentModes = device.getSurfacePresentModesKHR(surface);

    return details;
}


vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

vk::PresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, Window& window) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {

        VkExtent2D actualExtent = window.framebufferSize();

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Renderer::createSwapChain(Window& window) {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	vk::SwapchainCreateInfoKHR createInfo(vk::SwapchainCreateFlagsKHR(), surface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace, extent, 1, vk::ImageUsageFlagBits::eColorAttachment);

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	createInfo.clipped = true;
	createInfo.oldSwapchain = nullptr;

	try {
		swapChain = device.createSwapchainKHR(createInfo);
	} catch (vk::SystemError & err) {
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		exit(-1);
	} catch (std::exception & err) {
		std::cout << "std::exception: " << err.what() << std::endl;
		exit(-1);
	} catch (...) {
		std::cout << "unknown error" << std::endl;
		exit(-1);
	}

	swapChainImages = device.getSwapchainImagesKHR(swapChain);
}

void Renderer::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		vk::ImageViewCreateInfo createInfo(vk::ImageViewCreateFlags(), swapChainImages[i], vk::ImageViewType::e2D, swapChainImageFormat, vk::ComponentMapping(vk::ComponentSwizzle::eIdentity,vk::ComponentSwizzle::eIdentity,vk::ComponentSwizzle::eIdentity,vk::ComponentSwizzle::eIdentity), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

		try {
			swapChainImageViews[i] = device.createImageView(createInfo);
		} catch (vk::SystemError & err) {
			std::cout << "vk::SystemError: " << err.what() << std::endl;
			exit(-1);
		} catch (std::exception & err) {
			std::cout << "std::exception: " << err.what() << std::endl;
			exit(-1);
		} catch (...) {
			std::cout << "unknown error" << std::endl;
			exit(-1);
		}
	}
}

void createGraphicsPipeline() {
	
}

void Renderer::tick() {

}
