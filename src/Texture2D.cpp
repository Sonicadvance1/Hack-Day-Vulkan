#include "Vulkan.h"
#include "Texture2D.h"
#include "Utils.h"

#include <string.h>

namespace Vulkan
{

static bool IsDepthFormat(VkFormat Format)
{
	// XXX: Pretty sure there are more depth formats than this
	if (Format == VK_FORMAT_D16_UNORM)
		return true;
	return false;
}

Texture2D::Texture2D(Vulkan::InstanceObject& Instance,
		VkExtent2D Dim, uint32_t Levels, uint32_t Layers,
		VkFormat Format, VkSampleCountFlagBits Samples,
		VkImageViewType ViewType, VkImageTiling Tiling,
		VkImageUsageFlags Usage, VkFlags Props)
	: mDim(Dim), mLevels(Levels), mLayers(Layers), mFormat(Format)
	, mSamples(Samples), mTiling(Tiling), mProps(Props)
	, mUsage(Usage)
{
	auto AspectMask = IsDepthFormat(mFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	const VkImageCreateInfo ImageInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = mFormat,
		.extent = { mDim.width, mDim.height, 1},
		.mipLevels = mLevels,
		.arrayLayers = mLayers,
		.samples = mSamples,
		.tiling = mTiling,
		.usage = mUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VkMemoryAllocateInfo MemAllocate =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = 0, // Placeholder
		.memoryTypeIndex = 0, // Placeholder
	};

	VkImageViewCreateInfo ViewCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = VK_NULL_HANDLE,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = mFormat,
		.components =
		{
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange =
		{
			.aspectMask = AspectMask,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};


	VkResult err;
	uint32_t MemTypeIndex = ~0U;
	VkMemoryRequirements MemRequirements;

	// Create image
	err = vkCreateImage(*Instance.GetDevice(), &ImageInfo, nullptr, &mImage);
	CHECK_ERR(err);

	// Get memory requirements
	vkGetImageMemoryRequirements(*Instance.GetDevice(), mImage, &MemRequirements);

	MemAllocate.allocationSize = MemRequirements.size;
	mAllocationSize = MemAllocate.allocationSize;
	MemTypeIndex = Util::MemoryTypeFromProperties(Instance, MemRequirements.memoryTypeBits, mProps);
	assert(MemTypeIndex != ~0U);

	// Allocate Memory
	err = vkAllocateMemory(*Instance.GetDevice(), &MemAllocate, nullptr, &mMemory);
	CHECK_ERR(err);

	// Bind Memory
	err = vkBindImageMemory(*Instance.GetDevice(), mImage, mMemory, 0);
	CHECK_ERR(err);

	// Create view
	ViewCreateInfo.image = mImage;
	err = vkCreateImageView(*Instance.GetDevice(), &ViewCreateInfo, nullptr, &mView);
	CHECK_ERR(err);
}

void Texture2D::CopyToTexture(Vulkan::InstanceObject& Instance, PNGLoader* Png)
{
	assert(GetTiling() == VK_IMAGE_TILING_LINEAR);
	assert(GetUsage() & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	assert(GetFlags() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	// This is really expecting you to know the format you're copying in from the PNG is the exact same as you'd expect
	auto AspectMask = IsDepthFormat(mFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	const VkImageSubresource SubResource =
	{
		.aspectMask = AspectMask,
		.mipLevel = 0,
		.arrayLayer = 0,
	};
	VkSubresourceLayout SubLayout{};

	vkGetImageSubresourceLayout(*Instance.GetDevice(), GetImage(), &SubResource, &SubLayout);

	VkResult err;
	void* Data;

	err = vkMapMemory(*Instance.GetDevice(), GetMemory(), 0, mAllocationSize, 0, &Data);
	CHECK_ERR(err);

	VkExtent2D Dim = GetDimensions();

	assert(Dim.width == Png->GetWidth() && Dim.height == Png->GetHeight());
	const std::vector<uint8_t> PNGData = Png->GetData();
	for (uint32_t y = 0; y < Dim.height; ++y)
	{
		void* Row = (void*)((uint8_t*)Data + SubLayout.rowPitch * y);
		memcpy(Row, &PNGData.at(y * Dim.width), Dim.width * 4);
	}

	vkUnmapMemory(*Instance.GetDevice(), GetMemory());

	// XXX: Transition to read-only?
}

void Texture2D::CopyFromTexture(Vulkan::InstanceObject& Instance, Texture2D *Texture)
{
	assert(Texture->GetDimensions() == GetDimensions());
	assert(Texture->GetFormat() == GetFormat());
	assert(Texture->GetUsage() == VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	assert(GetUsage() == VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	// Transition images to the optimal transfer types
	VkImageLayout OldSrcLayout, OldDstLayout;
	OldSrcLayout = Texture->GetLayout();
	OldDstLayout = GetLayout();

	Texture->TransitionImageFormat(Instance, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	TransitionImageFormat(Instance, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	auto AspectMask = IsDepthFormat(mFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	VkExtent2D Dim = Texture->GetDimensions();
	VkImageCopy CopyRegion =
	{
		.srcSubresource =
		{
			AspectMask, 0, 0, 1
		},
		.srcOffset =
		{
			0, 0, 0
		},
		.dstSubresource =
		{
			AspectMask, 0, 0, 1
		},
		.dstOffset =
		{
			0, 0, 0
		},
		.extent =
		{
			Dim.width, Dim.height, 1
		},
	};

	vkCmdCopyImage(Instance.mSetupCommand,
	               Texture->GetImage(), Texture->GetLayout(),
			   GetImage(), GetLayout(),
			   1, &CopyRegion);

	// Transition back to the Old format for the destination
	// XXX: Should we skip the source since it would most likely be just a staging buffer?
	// Texture->TransitionImageFormat(Instance, OldSrcLayout);
	TransitionImageFormat(Instance, OldDstLayout);
}

void Texture2D::TransitionImageFormat(Vulkan::InstanceObject& Instance, VkImageLayout NewLayout)
{
	VkResult err;
	if (Instance.mSetupCommand == VK_NULL_HANDLE)
	{
		// If we've never called this before, let's generate it
		const VkCommandBufferAllocateInfo CommandInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = Instance.mCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		err = vkAllocateCommandBuffers(*Instance.GetDevice(), &CommandInfo, &Instance.mSetupCommand);
		CHECK_ERR(err);
	}

	const VkCommandBufferInheritanceInfo CommandBufferInherentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext = nullptr,
		.renderPass = VK_NULL_HANDLE,
		.subpass = 0,
		.framebuffer = VK_NULL_HANDLE,
		.occlusionQueryEnable = VK_FALSE,
		.queryFlags = 0,
		.pipelineStatistics = 0,
	};

	const VkCommandBufferBeginInfo CommandBufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = &CommandBufferInherentInfo,
	};

	err = vkBeginCommandBuffer(Instance.mSetupCommand, &CommandBufferInfo);
	CHECK_ERR(err);

	auto AspectMask = IsDepthFormat(mFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	// Let's actually set the image layout now
	VkImageMemoryBarrier MemoryBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = mLayout,
		.newLayout = NewLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = mImage,
		.subresourceRange = {AspectMask, 0, 1, 0, 1},
	};

	switch (NewLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		MemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		MemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		MemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		break;
	default:
		// Do nothing
		break;
	}

	VkPipelineStageFlags SrcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkPipelineStageFlags DstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(Instance.mSetupCommand, SrcStage, DstStage, 0, 0, nullptr,
	                     0, nullptr, 1, &MemoryBarrier);

	err = vkEndCommandBuffer(Instance.mSetupCommand);
	CHECK_ERR(err);

	mLayout = NewLayout;
}

Texture2D::~Texture2D()
{
}

Sampler::Sampler(Vulkan::InstanceObject& Instance,
	std::unique_ptr<Texture2D> Texture)
	: mTexture(std::move(Texture))
{
	VkResult err;
	auto AspectMask = IsDepthFormat(mTexture->GetFormat()) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	const VkSamplerCreateInfo SamplerInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_NEVER,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		.unnormalizedCoordinates = VK_FALSE,
	};

	VkImageViewCreateInfo ViewCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = mTexture->GetImage(),
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = mTexture->GetFormat(),
		.components =
		{
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange =
		{
			.aspectMask = AspectMask,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	// Create sampler
	err = vkCreateSampler(*Instance.GetDevice(), &SamplerInfo, nullptr, &mSampler);
	CHECK_ERR(err);
	printf("Created Sampler %p\n", this);
}

}
