#include "Vulkan.h"

#include <algorithm>
#include <array>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define GET_INSTANCE_ADDR(inst, entrypoint) \
{ \
	inst->entrypoint = \
		(PFN_vk##entrypoint)vkGetInstanceProcAddr(inst->GetInst(), "vk"#entrypoint); \
	if (inst->entrypoint == nullptr) \
	{ \
		fprintf(stderr, "vkGetInstanceProcAddress failed to find vk" #entrypoint "\n"); \
	} \
}
#define GET_INSTANCE_ADDRS(inst, entrypoint, suffix) \
{ \
	inst->entrypoint = \
		(PFN_vk##entrypoint##suffix)vkGetInstanceProcAddr(inst->GetInst(), "vk"#entrypoint #suffix); \
	if (inst->entrypoint == nullptr) \
	{ \
		fprintf(stderr, "vkGetInstanceProcAddress failed to find vk" #entrypoint "\n"); \
	} \
}

#define GET_DEVICE_ADDR(inst, entrypoint) \
{ \
	inst.entrypoint = \
		(PFN_vk##entrypoint)vkGetDeviceProcAddr(*inst.GetDevice(), "vk"#entrypoint); \
	if (inst.entrypoint == nullptr) \
	{ \
		fprintf(stderr, "vkGetDeviceProcAddr failed to find vk" #entrypoint "\n"); \
	} \
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugFunction(VkDebugReportFlagsEXT MsgFlags, VkDebugReportObjectTypeEXT ObjType,
		  uint64_t SourceObj, size_t loc, int32_t MsgCode,
		  const char* LayerPrefix, const char* Msg, void* UserData)
{
	std::string LogLevel;
	if (MsgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		LogLevel = "ERROR";
	else if (MsgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		LogLevel = "WARNING";
	else
		LogLevel = "OTHER";

	fprintf(stderr, "%s: [%s] Code %d : %s\n",
	        LogLevel.c_str(), LayerPrefix, MsgCode, Msg);
	return false;
}

namespace Vulkan
{
	InstanceObject::InstanceObject(bool validate)
		: mValidate(validate)
	{
		std::vector<const char*> Extensions;
		std::vector<VkExtensionProperties> InstanceExtensions;
		const char** RequiredExtensions;
		uint32_t RequiredCount;

		Vulkan::GetInstanceExtensions(&InstanceExtensions);
		RequiredExtensions = Vulkan::GetRequiredExtensions(&RequiredCount);

		for (int i = 0; i < RequiredCount; ++i)
		{
			mInstanceExtensionNames[mInstanceExtensions++] = RequiredExtensions[i];
		}

		if (mValidate)
		{
			for (const auto& ext : InstanceExtensions)
			{
				if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
				            ext.extensionName))
				{
					mInstanceExtensionNames[mInstanceExtensions++] = ext.extensionName;
					break;
				}
			}
		}

		mAppInfo =
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "VulkanTest",
			.applicationVersion = 0,
			.pEngineName = "VulkanTest",
			.engineVersion = 0,
			.apiVersion = VK_API_VERSION_1_0,
		};

		mCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &mAppInfo,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = mInstanceExtensions,
			.ppEnabledExtensionNames = mInstanceExtensionNames,
		};

		VkResult err;
		err = vkCreateInstance(&mCreateInfo, nullptr, &mInst);
		if (err == VK_ERROR_INCOMPATIBLE_DRIVER)
		{
			fprintf(stderr, "Couldn't find a Vulkan ICD\n");
		}
		else if (err == VK_ERROR_EXTENSION_NOT_PRESENT)
		{
			fprintf(stderr, "Couldn't find an extension\n");
		}
		else if (err)
		{
			fprintf(stderr, "vkCreateInstance failed\n");
		}

		GET_INSTANCE_ADDR(this, GetPhysicalDeviceSurfaceCapabilitiesKHR);
		GET_INSTANCE_ADDR(this, GetPhysicalDeviceSurfaceFormatsKHR);
		GET_INSTANCE_ADDR(this, GetPhysicalDeviceSurfacePresentModesKHR);
		GET_INSTANCE_ADDR(this, GetPhysicalDeviceSurfaceSupportKHR);
		GET_INSTANCE_ADDRS(this, CreateDebugReportCallback, EXT);

		if (!CreateDebugReportCallback)
		{
			fprintf(stderr, "Couldn't find vkCreateDebugReportCallbackEXT\n");
		}

		VkDebugReportCallbackCreateInfoEXT DebugCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
			.pNext = nullptr,
			.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
			.pfnCallback = DebugFunction,
			.pUserData = nullptr,
		};

		err = CreateDebugReportCallback(mInst, &DebugCreateInfo, nullptr, &mMsgCallback);
		assert(!err);
	}

	InstanceObject::~InstanceObject()
	{
	}

	void InstanceObject::SetDeviceExtensions()
	{
		std::vector<VkExtensionProperties> Extensions;
		Vulkan::GetDeviceExtensions(*this, &Extensions);
		for (auto ext : Extensions)
		{
			if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			            ext.extensionName))
			{
				mDeviceExtensionNames[mDeviceExtensions++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
				break;
			}
		}
	}

	const char** GetRequiredExtensions(uint32_t *count)
	{
		return glfwGetRequiredInstanceExtensions(count);
	}

	// Instance information
	void GetInstanceValidationLayers(std::vector<VkLayerProperties>* Layers)
	{
		VkResult err;
		uint32_t InstanceLayerCount;
		err = vkEnumerateInstanceLayerProperties(&InstanceLayerCount, nullptr);
		assert(!err);

		Layers->resize(InstanceLayerCount);
		err = vkEnumerateInstanceLayerProperties(&InstanceLayerCount, &Layers->at(0));
		assert(!err);
	}

	void GetInstanceExtensions(std::vector<VkExtensionProperties>* Extensions)
	{
		VkResult err;
		uint32_t InstanceExtensionCount;
		err = vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionCount, nullptr);
		assert(!err);

		Extensions->resize(InstanceExtensionCount);
		err = vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionCount, &Extensions->at(0));
		assert(!err);
	}

	// Device information
	uint32_t GetGPUCount(InstanceObject& inst)
	{
		VkResult err;
		uint32_t GPUCount;
		err = vkEnumeratePhysicalDevices(inst.GetInst(), &GPUCount, nullptr);
		assert(!err);
		return GPUCount;
	}

	void UseGPU(InstanceObject& inst, uint32_t index)
	{
		VkResult err;
		uint32_t GPUCount;
		err = vkEnumeratePhysicalDevices(inst.GetInst(), &GPUCount, nullptr);
		assert(!err);
		assert(index < GPUCount);
		std::vector<VkPhysicalDevice> GPUs(GPUCount);
		err = vkEnumeratePhysicalDevices(inst.GetInst(), &GPUCount, &GPUs[0]);
		assert(!err);
		inst.SetGPU(GPUs[0]);
	}

	void CreateDevice(InstanceObject& inst)
	{
		VkResult err;
		float queue_priorities[1] = { 0.0 };
		uint32_t ExtensionCount;
		inst.SetDeviceExtensions();
		const char** Extensions = inst.GetDeviceExtensions(&ExtensionCount);

		std::vector<VkLayerProperties> Layers;
		GetInstanceValidationLayers(&Layers);

		uint32_t LayerCount = 0;
		const char* LayerNames[64];

		std::array<std::string, 10> mValidationLayers =
		{
			"VK_LAYER_LUNARG_object_tracker",
			"VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_LUNARG_core_validation",
			"VK_LAYER_LUNARG_standard_validation",
			"VK_LAYER_GOOGLE_unique_objects",
		};

		printf("===========================\n");
		printf("We have %zd layers to test\n", Layers.size());
		for (const auto& layer : Layers)
		{
			if (std::find(mValidationLayers.begin(),
			              mValidationLayers.end(),
					  std::string(layer.layerName)) != mValidationLayers.end())
			{
				printf("Enabled layer %d:%s\n", LayerCount, layer.layerName);
				LayerNames[LayerCount++] = layer.layerName;
			}
		}
		printf("===========================\n");

		const VkDeviceQueueCreateInfo DeviceCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueFamilyIndex = inst.GetPresentQueueIndex(),
			.queueCount = 1,
			.pQueuePriorities = queue_priorities,
		};

		VkDeviceCreateInfo DeviceInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &DeviceCreateInfo,
			.enabledLayerCount = LayerCount,
			.ppEnabledLayerNames = LayerNames,
			.enabledExtensionCount = ExtensionCount,
			.ppEnabledExtensionNames = Extensions,
		};

		err = vkCreateDevice(inst.GetGPU(), &DeviceInfo, nullptr, inst.GetDevice());

		assert(!err);

		GET_DEVICE_ADDR(inst, CreateSwapchainKHR);
		GET_DEVICE_ADDR(inst, DestroySwapchainKHR);
		GET_DEVICE_ADDR(inst, GetSwapchainImagesKHR);
		GET_DEVICE_ADDR(inst, AcquireNextImageKHR);
		GET_DEVICE_ADDR(inst, QueuePresentKHR);

		vkGetDeviceQueue(*inst.GetDevice(), inst.GetPresentQueueIndex(), 0, inst.GetQueue());

	}

	void GetDeviceValidationLayers(InstanceObject& inst, std::vector<VkLayerProperties>* Layers)
	{
		VkResult err;
		uint32_t DeviceLayerCount;
		err = vkEnumerateDeviceLayerProperties(inst.GetGPU(), &DeviceLayerCount, nullptr);
		assert(!err);

		if (DeviceLayerCount == 0)
			return;

		Layers->resize(DeviceLayerCount);
		err = vkEnumerateDeviceLayerProperties(inst.GetGPU(), &DeviceLayerCount, &Layers->at(0));
		assert(!err);
	}

	void GetDeviceExtensions(InstanceObject& inst, std::vector<VkExtensionProperties>* Extensions)
	{
		VkResult err;
		uint32_t DeviceExtensionCount;
		err = vkEnumerateDeviceExtensionProperties(inst.GetGPU(), nullptr, &DeviceExtensionCount, nullptr);
		assert(!err);

		if (DeviceExtensionCount == 0)
			return;

		Extensions->resize(DeviceExtensionCount);
		err = vkEnumerateDeviceExtensionProperties(inst.GetGPU(), nullptr, &DeviceExtensionCount, &Extensions->at(0));
		assert(!err);
	}

	void GetDeviceQueueProperties(InstanceObject& inst, std::vector<VkQueueFamilyProperties>* Queues)
	{
		uint32_t DeviceQueueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(inst.GetGPU(), &DeviceQueueCount, nullptr);

		if (DeviceQueueCount == 0)
			return;

		Queues->resize(DeviceQueueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(inst.GetGPU(), &DeviceQueueCount, &Queues->at(0));
	}

	void GetDeviceProperties(InstanceObject& inst)
	{
		vkGetPhysicalDeviceProperties(inst.GetGPU(), inst.GetGPUProp());
	}

	void GetMemoryProperties(InstanceObject& inst)
	{
		vkGetPhysicalDeviceMemoryProperties(inst.GetGPU(), inst.GetMemProp());
	}

	////////////////////////////////////////////////
	// Surface information
	////////////////////////////////////////////////
	void CreateWindowSurface(InstanceObject& inst, GLFWwindow* win)
	{
		// Creates our WSI surface
		glfwCreateWindowSurface(inst.GetInst(), win, nullptr, inst.GetSurface());
	}

	VkBool32 QueueSupportsPresent(InstanceObject& inst, uint32_t index)
	{
		VkBool32 SupportsPresent;
		inst.GetPhysicalDeviceSurfaceSupportKHR(inst.GetGPU(), index, *inst.GetSurface(), &SupportsPresent);
		return SupportsPresent;
	}

	void GetSurfaceFormats(InstanceObject& inst, std::vector<VkSurfaceFormatKHR>* Formats)
	{
		VkResult err;
		uint32_t SurfaceFormatCount;
		err = inst.GetPhysicalDeviceSurfaceFormatsKHR(inst.GetGPU(), *inst.GetSurface(), &SurfaceFormatCount, nullptr);
		assert(!err);

		Formats->resize(SurfaceFormatCount);
		err = inst.GetPhysicalDeviceSurfaceFormatsKHR(inst.GetGPU(), *inst.GetSurface(), &SurfaceFormatCount, &Formats->at(0));
		assert(!err);
	}

	////////////////////////////////////////////////
	// Pools
	////////////////////////////////////////////////
	void CreateCommandPool(InstanceObject& inst)
	{
		VkResult err;
		const VkCommandPoolCreateInfo CommandPoolCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = inst.GetPresentQueueIndex(),
		};

		err = vkCreateCommandPool(*inst.GetDevice(), &CommandPoolCreateInfo, nullptr, &inst.mCommandPool);
		assert(!err);

		const VkCommandBufferAllocateInfo CommandBufferAllocateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = inst.mCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		err = vkAllocateCommandBuffers(*inst.GetDevice(), &CommandBufferAllocateInfo, &inst.mDrawCommand);
		assert(!err);

		err = vkAllocateCommandBuffers(*inst.GetDevice(), &CommandBufferAllocateInfo, &inst.mSetupCommand);
		CHECK_ERR(err);
	}

	void SubmitSetupQueue(InstanceObject& inst)
	{
		VkResult err;
		VkFence NullFence = VK_NULL_HANDLE;
		VkSubmitInfo SubmitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &inst.mSetupCommand,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr,
		};

		err = vkQueueSubmit(*inst.GetQueue(), 1, &SubmitInfo, NullFence);
		CHECK_ERR(err);

		err = vkQueueWaitIdle(*inst.GetQueue());
		CHECK_ERR(err);

	}

	////////////////////////////////////////////////
	// SwapChain
	////////////////////////////////////////////////
	void CreateSwapChain(InstanceObject& inst)
	{
		VkResult err;
		VkSwapchainKHR OldSwapChain = inst.mSwapChain;
		VkSurfaceCapabilitiesKHR SurfaceCaps;
		err = inst.GetPhysicalDeviceSurfaceCapabilitiesKHR(inst.GetGPU(), *inst.GetSurface(), &SurfaceCaps);
		assert(!err);

		VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;

		uint32_t SwapChainImageCount = SurfaceCaps.minImageCount + 1;

		// Apparently we can run in to an issue where min = max
		// Settle for max
		if (SurfaceCaps.maxImageCount > 0)
			SwapChainImageCount = std::min(SwapChainImageCount, SurfaceCaps.maxImageCount);

		const VkSwapchainCreateInfoKHR SwapChain =
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = *inst.GetSurface(),
			.minImageCount = SwapChainImageCount,
			.imageFormat = inst.GetSurfaceFormat(),
			.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, // XXX: This should be queried
			.imageExtent = { SurfaceCaps.currentExtent.width, SurfaceCaps.currentExtent.height },
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.preTransform = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = PresentMode,
			.clipped = true, // XXX: wtf does this even mean?
			.oldSwapchain = OldSwapChain,
		};

		// Create our swpa chain
		err = inst.CreateSwapchainKHR(*inst.GetDevice(), &SwapChain, nullptr, &inst.mSwapChain);
		assert(!err);
		assert(OldSwapChain == VK_NULL_HANDLE); // XXX: Support

		std::vector<VkImage> SwapChainImages;
		// Get our swap chain image count
		err = inst.GetSwapchainImagesKHR(*inst.GetDevice(), inst.mSwapChain, &SwapChainImageCount, nullptr);
		assert(!err);

		SwapChainImages.resize(SwapChainImageCount);
		inst.mSwapChainBuffers.resize(SwapChainImageCount);
		err = inst.GetSwapchainImagesKHR(*inst.GetDevice(), inst.mSwapChain, &SwapChainImageCount, &SwapChainImages[0]);
		assert(!err);

		// Generate the views of our swap chain buffers
		for (int i = 0; i < SwapChainImageCount; ++i)
		{
			VkImageViewCreateInfo ImageView =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = SwapChainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = inst.GetSurfaceFormat(),
				.components =
					{
						.r = VK_COMPONENT_SWIZZLE_IDENTITY,
						.g = VK_COMPONENT_SWIZZLE_IDENTITY,
						.b = VK_COMPONENT_SWIZZLE_IDENTITY,
						.a = VK_COMPONENT_SWIZZLE_IDENTITY,
					},
				.subresourceRange = // XXX: What is the aspect mask?
					{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount= 1,
					},
			};

			err = vkCreateImageView(*inst.GetDevice(), &ImageView, nullptr, &inst.mSwapChainBuffers[i].mView);
			assert(!err);
		}

		inst.mCurrentSwapBuffer = 0;
	}
}
