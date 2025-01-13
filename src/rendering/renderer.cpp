#include <rendering/renderer.hpp>
#include <rendering/window.hpp>
#include <rendering/mesh.hpp>
#include <rendering/camera.hpp>
#include <assets/assets.hpp>
#include <assets/shaders.hpp>
#include <iostream>
#include <set>
#include <limits>
#include <algorithm>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

Renderer::Renderer(Window& window) {
	createInstance();
	surface = window.createSurface(instance);
	pickPhysicalDevice();
	createDevice();
	createSwapChain(window);
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}

Renderer::~Renderer() {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		device.destroyBuffer(uniformBuffers[i]);
		device.freeMemory(uniformBuffersMemory[i]);
    }

	device.destroyDescriptorPool(descriptorPool);
	device.destroyDescriptorSetLayout(descriptorSetLayout);
	device.destroyBuffer(indexBuffer);
	device.freeMemory(indexBufferMemory);
	device.destroyBuffer(vertexBuffer);
	device.freeMemory(vertexBufferMemory);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		device.destroySemaphore(renderFinishedSemaphores[i]);
		device.destroySemaphore(imageAvailableSemaphores[i]);
		device.destroyFence(inFlightFences[i]);
	}

	device.destroyCommandPool(commandPool);

	for (auto framebuffer : swapChainFramebuffers) {
		device.destroyFramebuffer(framebuffer);
    }

	device.destroyPipeline(graphicsPipeline);
	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderPass);

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

void Renderer::createGraphicsPipeline() {
	vk::ShaderModule vertex = createShaderModule(Identifier("core", "vertex"), ShaderType::Vertex, device);
	vk::ShaderModule fragment = createShaderModule(Identifier("core", "fragment"), ShaderType::Fragment, device);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, vertex, "main");
	vk::PipelineShaderStageCreateInfo fragShaderStageInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, fragment, "main");

	vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	vk::PipelineDynamicStateCreateInfo dynamicState(vk::PipelineDynamicStateCreateFlags(), dynamicStates);

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo(vk::PipelineVertexInputStateCreateFlags(),0, nullptr, 0, nullptr);

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly(vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList, false);

	vk::Viewport viewport(0.0f, 0.0f, (float) swapChainExtent.width, (float) swapChainExtent.height, 0.0f, 1.0f);

	vk::Rect2D scissor({0, 0}, swapChainExtent);

	vk::PipelineViewportStateCreateInfo viewportState(vk::PipelineViewportStateCreateFlags(), {viewport}, {scissor});

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = false;
	rasterizer.rasterizerDiscardEnable = false;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = false;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.sampleShadingEnable = false;
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = false;
	multisampling.alphaToOneEnable = false;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendAttachment.blendEnable = false;
	colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.logicOpEnable = false;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; 
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo(vk::PipelineLayoutCreateFlags(), {}, {});

	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	try {
		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
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

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;

	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = -1;

	try {
		vk::Result result;
		std::tie(result, graphicsPipeline) = device.createGraphicsPipeline(nullptr, pipelineInfo);
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

	device.destroyShaderModule(vertex);
	device.destroyShaderModule(fragment);
}

void Renderer::createRenderPass() {
	vk::AttachmentDescription colorAttachment;
	colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	vk::RenderPassCreateInfo renderPassInfo(vk::RenderPassCreateFlags(), {colorAttachment}, {subpass});

	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = vk::AccessFlags();
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	try {
		renderPass = device.createRenderPass(renderPassInfo);
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

void Renderer::createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vk::ImageView attachments[] = {
			swapChainImageViews[i]
		};

		vk::FramebufferCreateInfo framebufferInfo;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		try {
			swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
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

void Renderer::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	try {
		commandPool = device.createCommandPool(poolInfo);
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

void Renderer::createCommandBuffers() {
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

	try {
		commandBuffers = device.allocateCommandBuffers(allocInfo);
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


void Renderer::recordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex) {
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlags();
	beginInfo.pInheritanceInfo = nullptr;

	try {
		buffer.begin(beginInfo);
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


	vk::ClearValue clearColor(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

	vk::Viewport viewport;
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	buffer.setViewport(0, 1, &viewport);
	
	vk::Rect2D scissor;
	scissor.offset = vk::Offset2D(0, 0);
	scissor.extent = swapChainExtent;
	buffer.setScissor(0, 1, &scissor);

	vk::Buffer vertexBuffers[] = {vertexBuffer};
	vk::DeviceSize offsets[] = {0};

	buffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
	buffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);
	buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, {descriptorSets[currentFrame]}, {});
	buffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	buffer.endRenderPass();

	try {
		buffer.end();
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

void Renderer::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	vk::SemaphoreCreateInfo semaphoreInfo;
	vk::FenceCreateInfo fenceInfo;
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	try {
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
			renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
			inFlightFences[i] = device.createFence(fenceInfo);
		}
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

void Renderer::tick(Window& window) {
	device.waitForFences(inFlightFences[currentFrame], true, UINT64_MAX);
	uint32_t imageIndex;

	vk::ResultValue<uint32_t> result = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame]);

	switch(result.result) {
		case vk::Result::eSuboptimalKHR:
		case vk::Result::eSuccess:
			imageIndex = result.value;
			break;
		case vk::Result::eErrorOutOfDateKHR:
			recreateSwapChain(window);
			return;
		default:
			throw std::runtime_error("failed to acquire swap chain image!");
	}

	device.resetFences(inFlightFences[currentFrame]);

	commandBuffers[currentFrame].reset(vk::CommandBufferResetFlags());
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	updateUniformBuffer(currentFrame);

	vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
	vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
	vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	try {
		graphicsQueue.submit({submitInfo}, inFlightFences[currentFrame]);
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

	vk::SwapchainKHR swapChains[] = {swapChain};
	vk::PresentInfoKHR presentInfo;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	switch(presentQueue.presentKHR(presentInfo)) {
		case vk::Result::eSuccess:
			break;
		case vk::Result::eSuboptimalKHR:
		case vk::Result::eErrorOutOfDateKHR:
			recreateSwapChain(window);
			break;
		default:
			throw std::runtime_error("failed to present swap chain image2!");
	}

	if (window.framebufferResized) {
		recreateSwapChain(window);
		window.framebufferResized = false;
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::cleanupSwapChain() {
	for (auto framebuffer : swapChainFramebuffers) {
		device.destroyFramebuffer(framebuffer);
    }

	for (auto view : swapChainImageViews) {
		device.destroyImageView(view);
	}

	device.destroySwapchainKHR(swapChain);
}

void Renderer::recreateSwapChain(Window& window) {
	device.waitIdle();

	cleanupSwapChain();
    createSwapChain(window);
    createImageViews();
    createFramebuffers();
}

void Renderer::end() {
	device.waitIdle();
}

void Renderer::createVertexBuffer() {
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	vk::DeviceMemory stagingBufferMemory;
	vk::Buffer stagingBuffer = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBufferMemory);

	uint8_t * pData = static_cast<uint8_t *>(device.mapMemory(stagingBufferMemory, 0, bufferSize));
    memcpy(pData, vertices.data(), bufferSize);
    device.unmapMemory(stagingBufferMemory);

	vertexBuffer = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexBufferMemory);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingBufferMemory);
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
	vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties)) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void Renderer::createIndexBuffer() {
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	vk::DeviceMemory stagingBufferMemory;
	vk::Buffer stagingBuffer = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBufferMemory);

	uint8_t * pData = static_cast<uint8_t *>(device.mapMemory(stagingBufferMemory, 0, bufferSize));
    memcpy(pData, indices.data(), bufferSize);
    device.unmapMemory(stagingBufferMemory);

	indexBuffer = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingBufferMemory);
}

vk::Buffer Renderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags flags, vk::MemoryPropertyFlags properties, vk::DeviceMemory& bufferMemory) {
	vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = flags;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	vk::Buffer buffer;

	try {
		buffer = device.createBuffer(bufferInfo);
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

	vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer);
	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	try {
		bufferMemory = device.allocateMemory(allocInfo);
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

	device.bindBufferMemory(buffer, bufferMemory, 0);

	return buffer;
}

void Renderer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
	vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

	vk::CommandBuffer commandBuffer;
	device.allocateCommandBuffers(&allocInfo, &commandBuffer);

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandBuffer.begin(beginInfo);

	vk::BufferCopy copyRegion;
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	graphicsQueue.submit(submitInfo, nullptr);
	graphicsQueue.waitIdle();

	device.freeCommandBuffers(commandPool, {commandBuffer});
}

void Renderer::createDescriptorSetLayout() {
	vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	
	vk::DescriptorSetLayoutCreateInfo layoutInfo;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	try {
		descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
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

void Renderer::createUniformBuffers() {
	vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffersMemory[i]);
		uniformBuffersMapped[i] = static_cast<uint8_t *>(device.mapMemory(uniformBuffersMemory[i], 0, bufferSize));
    }
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);

	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Renderer::createDescriptorPool() {
	vk::DescriptorPoolSize poolSize;
	poolSize.type = vk::DescriptorType::eUniformBuffer;
	poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	try {
		descriptorPool = device.createDescriptorPool(poolInfo);
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

void Renderer::createDescriptorSets() {
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	try {
		descriptorSets = device.allocateDescriptorSets(allocInfo);
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

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		vk::WriteDescriptorSet descriptorWrite;
		descriptorWrite.dstSet = descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;
		
		device.updateDescriptorSets({descriptorWrite}, {});
	}
}
