#pragma once

#include "Texture2D.h"
#include "VertexInfo.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assert.h>
#include <memory>
#include <vector>

#define CHECK_ERR(err)\
{\
	if(err)\
	{\
		fprintf(stderr, "Got an error of %d(0x%x)\n", err, err);\
		assert(0);\
	}\
}

namespace Vulkan
{
	class InstanceObject
	{
	public:
		InstanceObject(bool validate);
		~InstanceObject();

		VkInstance GetInst() { return mInst; }
		VkPhysicalDevice GetGPU() { return mGPU; }

		// Properties
		VkPhysicalDeviceProperties* GetGPUProp() { return &mGPUProp; }
		VkSurfaceKHR* GetSurface() { return &mSurface; }
		void SetPresentQueueIndex(uint32_t index) { mPresentQueue = index; }
		uint32_t GetPresentQueueIndex() { return mPresentQueue; }
		void SetQueue(VkQueue Queue) { mQueue = Queue; }
		VkQueue* GetQueue() { return &mQueue; }

		void SetGPU(VkPhysicalDevice GPU) { mGPU = GPU; }
		VkDevice* GetDevice() { return &mDevice; }

		void SetSurfaceFormat(VkFormat Format) { mFormat = Format; }
		VkFormat GetSurfaceFormat() { return mFormat; }

		void SetMemProp(VkPhysicalDeviceMemoryProperties Prop) { mMemProp = Prop; }
		VkPhysicalDeviceMemoryProperties* GetMemProp() { return &mMemProp; }

		// Extensions
		const char** GetInstanceExtensions(uint32_t* count)
		{
			*count = mInstanceExtensions;
			return mInstanceExtensionNames;
		}

		void SetDeviceExtensions();
		const char** GetDeviceExtensions(uint32_t* count)
		{
			*count = mDeviceExtensions;
			return mDeviceExtensionNames;
		}


		// Instance Function pointers
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
			GetPhysicalDeviceSurfaceCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR
			GetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
			GetPhysicalDeviceSurfacePresentModesKHR;
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR
			GetPhysicalDeviceSurfaceSupportKHR;
		PFN_vkCreateDebugReportCallbackEXT
			CreateDebugReportCallback;


		// Device function pointers
		PFN_vkCreateSwapchainKHR CreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR DestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR;
		PFN_vkAcquireNextImageKHR AcquireNextImageKHR;
		PFN_vkQueuePresentKHR QueuePresentKHR;

		// Command pool
		VkCommandPool mCommandPool;

		// Command Buffer
		VkCommandBuffer mDrawCommand{};
		VkCommandBuffer mSetupCommand{}; // For initialization

		// Swap chain
		VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
		struct SwapChainBuffers
		{
			VkImage mImage;
			VkCommandBuffer mCommandBuffer;
			VkImageView mView;
		};
		std::vector<SwapChainBuffers> mSwapChainBuffers;
		uint32_t mCurrentSwapBuffer;

		// Render pass
		VkRenderPass mRenderPass;

		// Framebuffers
		std::vector<VkFramebuffer> mFramebuffers;

		// Pipeline
		VkPipeline mPipeline;
		VkPipelineCache mPipelineCache;

		// Descriptor layouts
		VkDescriptorSet mDescriptorSet;
		VkDescriptorPool mDescriptorPool;
		VkDescriptorSetLayout mDescriptorLayout;
		VkPipelineLayout mPipelineLayout;

		// Uniform buffer
		struct
		{
			glm::mat4 projectionMatrix;
			glm::mat4 modelMatrix;
			glm::mat4 viewMatrix;
		} mUBOData;
		std::unique_ptr<UniformBuffer> mUBO;

		// Vertices
		std::unique_ptr<VertexBuffer> mVertices;
		uint32_t mVerticeCount;

		// Indices
		std::unique_ptr<IndicesBuffer> mIndices;

		// Depth texture
		std::unique_ptr<Texture2D> mDepth;

		// Textures
		std::unique_ptr<Sampler> mSampler;

		// Debug callback
		VkDebugReportCallbackEXT mMsgCallback;

	private:
		// Provides information to the VkCreateInstance
		VkApplicationInfo mAppInfo;
		VkInstanceCreateInfo mCreateInfo;

		// The Instance
		VkInstance mInst;

		// Physical device
		VkPhysicalDevice mGPU;
		VkPhysicalDeviceProperties mGPUProp;

		// WSI surface
		VkSurfaceKHR mSurface;

		// Device object
		VkDevice mDevice;

		// Extensions
		const char *mInstanceExtensionNames[64];
		uint32_t mInstanceExtensions = 0;
		const char *mDeviceExtensionNames[64];
		uint32_t mDeviceExtensions = 0;

		// Validation
		bool mValidate;

		// Present Queue index
		uint32_t mPresentQueue = ~0U;
		VkQueue mQueue;

		// Surface format
		VkFormat mFormat;

		VkPhysicalDeviceMemoryProperties mMemProp;
	};

	const char** GetRequiredExtensions(uint32_t* count);

	////////////////////////////////////////////////
	// Instance information
	////////////////////////////////////////////////
	void GetInstanceValidationLayers(std::vector<VkLayerProperties>* Layers);
	void GetInstanceExtensions(std::vector<VkExtensionProperties>* Extensions);

	////////////////////////////////////////////////
	// Device information
	////////////////////////////////////////////////
	uint32_t GetGPUCount(InstanceObject& inst);
	// Used to bind a GPU to your instance
	void UseGPU(InstanceObject& inst, uint32_t index);
	// Use this to create a device
	void CreateDevice(InstanceObject& inst);

	// Only use these once you have bound a GPU to your instance!
	void GetDeviceValidationLayers(InstanceObject& inst, std::vector<VkLayerProperties>* Layers);
	void GetDeviceExtensions(InstanceObject& inst, std::vector<VkExtensionProperties>* Extensions);
	void GetDeviceQueueProperties(InstanceObject& inst, std::vector<VkQueueFamilyProperties>* Queues);
	void GetDeviceProperties(InstanceObject& inst);
	void GetMemoryProperties(InstanceObject& inst);

	////////////////////////////////////////////////
	// Surface information
	////////////////////////////////////////////////
	// Used to bind a surface to your instance
	void CreateWindowSurface(InstanceObject& inst, GLFWwindow *win);

	// Only use these once you have bound a surface to your instance!
	VkBool32 QueueSupportsPresent(InstanceObject& inst, uint32_t index);
	void GetSurfaceFormats(InstanceObject& inst, std::vector<VkSurfaceFormatKHR>* Formats);

	////////////////////////////////////////////////
	// Pools
	////////////////////////////////////////////////
	void CreateCommandPool(InstanceObject& inst);
	void SubmitSetupQueue(InstanceObject& inst);

	////////////////////////////////////////////////
	// SwapChain
	////////////////////////////////////////////////
	void CreateSwapChain(InstanceObject& inst);
}
