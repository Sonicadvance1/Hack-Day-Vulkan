#pragma once

#include "PNGLoader.h"

#include <vulkan/vulkan.h>
#include <memory>

namespace Vulkan
{
class InstanceObject;

class Texture2D
{
public:
	Texture2D(Vulkan::InstanceObject& Instance,
	          VkExtent2D Dim, uint32_t Levels, uint32_t Layers,
	          VkFormat Format, VkSampleCountFlagBits Samples,
		    VkImageViewType ViewType, VkImageTiling Tiling,
		    VkImageUsageFlags Usage, VkFlags Props);

	~Texture2D();

	static std::unique_ptr<Texture2D> CreateGPU(Vulkan::InstanceObject& Instance,
		VkExtent2D Dim, uint32_t Levels, uint32_t Layers,
		VkFormat Format, VkSampleCountFlagBits Samples,
		VkImageViewType ViewType, VkImageTiling Tiling,
		VkImageUsageFlags Usage)
	{
		return std::make_unique<Texture2D>(Instance, Dim, Levels, Layers, Format, Samples, ViewType, Tiling, Usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	static std::unique_ptr<Texture2D> CreateHost(Vulkan::InstanceObject& Instance,
		VkExtent2D Dim, uint32_t Levels, uint32_t Layers,
		VkFormat Format, VkSampleCountFlagBits Samples,
		VkImageViewType ViewType, VkImageTiling Tiling,
		VkImageUsageFlags Usage)
	{
		return std::make_unique<Texture2D>(Instance, Dim, Levels, Layers, Format, Samples, ViewType, Tiling, Usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	}

	// Copy from a PNG to this texture
	// Must be linear and host visible
	void CopyToTexture(Vulkan::InstanceObject& Instance, PNGLoader* Png);

	// Copy from one Texture2D to this one
	// Useful for copy staging to tiling buffers
	void CopyFromTexture(Vulkan::InstanceObject& Instance, Texture2D *Texture);
	void TransitionImageFormat(Vulkan::InstanceObject& Instance, VkImageLayout NewLayout);

	// Device objects
	VkImage GetImage() const { return mImage; }
	VkDeviceMemory GetMemory() const { return mMemory; }
	VkImageView GetView() const { return mView; }

	// Information
	VkExtent2D GetDimensions() const { return mDim; }
	uint32_t GetLevels() const { return mLevels; }
	uint32_t GetLayers() const { return mLayers; }
	VkFormat GetFormat() const { return mFormat; }
	VkSampleCountFlagBits GetSamples() const { return mSamples; }
	VkImageLayout GetLayout() const { return mLayout; }
	VkImageTiling GetTiling() const { return mTiling; }
	VkFlags GetFlags() const { return mProps; }
	VkImageUsageFlags GetUsage() const { return mUsage; }

private:
	VkExtent2D mDim;
	uint32_t mLevels, mLayers;
	VkFormat mFormat;
	VkSampleCountFlagBits mSamples;
	VkImageLayout mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageTiling mTiling;
	VkFlags mProps;
	VkImageUsageFlags mUsage;
	uint32_t mAllocationSize;

	VkImage mImage;
	VkDeviceMemory mMemory;
	VkImageView mView;

};

class Sampler
{
public:

	Sampler(Vulkan::InstanceObject& Instance,
	        std::unique_ptr<Texture2D> Texture);

	~Sampler() {}

	// Device objects
	VkSampler GetSampler() const { return mSampler; }

	// Information
	Texture2D* GetTexture() const { return mTexture.get(); }

private:
	std::unique_ptr<Texture2D> mTexture;

	VkSampler mSampler;
};
}
