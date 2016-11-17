#include <string.h>

#include "Utils.h"
#include "Vulkan.h"
#include "VertexInfo.h"

namespace Vulkan
{
	VkPipelineVertexInputStateCreateInfo VertexBuffer::mVI;

	VertexBuffer::VertexBuffer(Vulkan::InstanceObject& Instance,
	                           const std::vector<float>& Vertices,
			               uint32_t BindingID,
	                           uint32_t Stride,
			               VkBufferUsageFlagBits Usage)
		: mBindingID(BindingID)
	{
		VkResult err;
		void *Data;
		uint32_t MemTypeIndex = ~0U;

		uint32_t BufferSize = Vertices.size() * sizeof(float);

		const VkBufferCreateInfo VerticesBufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = BufferSize,
			.usage = Usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		VkMemoryAllocateInfo MemAllocate =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = 0,
			.memoryTypeIndex = 0,
		};

		VkMemoryRequirements MemRequirements{};

		// Generate the Vertices buffer
		err = vkCreateBuffer(*Instance.GetDevice(), &VerticesBufferInfo, nullptr, &mBuffer);
		CHECK_ERR(err);

		// Get the buffer memory requirements
		vkGetBufferMemoryRequirements(*Instance.GetDevice(), mBuffer, &MemRequirements);
		CHECK_ERR(err);

		MemAllocate.allocationSize = MemRequirements.size;

		// Find a memory type matching the properties we need
		MemTypeIndex = Util::MemoryTypeFromProperties(Instance, MemRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		printf("MemTypeIndex %d\n", MemTypeIndex);
		assert(MemTypeIndex != ~0U);

		// Allocate the memory
		err = vkAllocateMemory(*Instance.GetDevice(), &MemAllocate, nullptr, &mMemory);
		CHECK_ERR(err);

		// Map this memory
		err = vkMapMemory(*Instance.GetDevice(), mMemory, 0, MemAllocate.allocationSize, 0, &Data);
		CHECK_ERR(err);

		// Upload the data array we have to the mapped memory
		memcpy(Data, &Vertices[0], BufferSize);

		// Unmap the memory
		vkUnmapMemory(*Instance.GetDevice(), mMemory);

		// Map the memory to the buffer
		err = vkBindBufferMemory(*Instance.GetDevice(), mBuffer, mMemory, 0);
		CHECK_ERR(err);

		// Setup the vertex bindings
		mVIBinding.binding = mBindingID;
		mVIBinding.stride = Stride;
		mVIBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// Setup the vertex input info
		mVI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		mVI.pNext = nullptr;
		mVI.flags = 0;
		mVI.vertexBindingDescriptionCount = 1;
		mVI.pVertexBindingDescriptions = &mVIBinding;
		mVI.pVertexAttributeDescriptions = mVIAttributes;
	}

	VertexBuffer::~VertexBuffer()
	{
	}

	void VertexBuffer::AddAttribute(VkVertexInputAttributeDescription VIAttribute)
	{
		memcpy(&mVIAttributes[mNumAttribs], &VIAttribute, sizeof(VkVertexInputAttributeDescription));

		mVI.vertexAttributeDescriptionCount = ++mNumAttribs;
	}

	IndicesBuffer::IndicesBuffer(Vulkan::InstanceObject& Instance,
	                             const std::vector<uint32_t>& Indices,
	                             VkBufferUsageFlagBits Usage)
	{
		VkResult err;
		void *Data;
		uint32_t MemTypeIndex = ~0U;
		const VkBufferCreateInfo IndicesBufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = Indices.size() * sizeof(uint32_t),
			.usage = Usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		VkMemoryAllocateInfo MemAllocate =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = 0,
			.memoryTypeIndex = 0,
		};

		VkMemoryRequirements MemRequirements{};

		// Generate the Indices buffer
		err = vkCreateBuffer(*Instance.GetDevice(), &IndicesBufferInfo, nullptr, &mBuffer);
		CHECK_ERR(err);

		// Get the buffer memory requirements
		vkGetBufferMemoryRequirements(*Instance.GetDevice(), mBuffer, &MemRequirements);
		CHECK_ERR(err);

		MemAllocate.allocationSize = MemRequirements.size;

		// Find a memory type matching the properties we need
		MemTypeIndex = Util::MemoryTypeFromProperties(Instance, MemRequirements.memoryTypeBits,
		                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		printf("MemTypeIndex %d\n", MemTypeIndex);
		assert(MemTypeIndex != ~0U);

		// Allocate the memory
		err = vkAllocateMemory(*Instance.GetDevice(), &MemAllocate, nullptr, &mMemory);
		CHECK_ERR(err);

		// Map this memory
		err = vkMapMemory(*Instance.GetDevice(), mMemory, 0, MemAllocate.allocationSize, 0, &Data);
		CHECK_ERR(err);

		// Upload the data array we have to the mapped memory
		memcpy(Data, &Indices[0], Indices.size() * sizeof(uint32_t));

		// Unmap the memory
		vkUnmapMemory(*Instance.GetDevice(), mMemory);

		// Map the memory to the buffer
		err = vkBindBufferMemory(*Instance.GetDevice(), mBuffer, mMemory, 0);
		CHECK_ERR(err);

		mCount = Indices.size();
	}

	IndicesBuffer::~IndicesBuffer()
	{
	}

	UniformBuffer::UniformBuffer(Vulkan::InstanceObject& Instance,
	                             uint32_t Size, VkMemoryPropertyFlagBits MemoryProperty)
	{
		VkResult err;
		void *Data;
		uint32_t MemTypeIndex = ~0U;

		const VkBufferCreateInfo UniformBufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = Size,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		VkMemoryAllocateInfo MemAllocate =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = 0,
			.memoryTypeIndex = 0,
		};

		VkMemoryRequirements MemRequirements{};

		// Create the Uniform buffer;
		err = vkCreateBuffer(*Instance.GetDevice(), &UniformBufferInfo, nullptr, &mBuffer);
		CHECK_ERR(err);

		// Get the buffer memory requirements
		vkGetBufferMemoryRequirements(*Instance.GetDevice(), mBuffer, &MemRequirements);
		CHECK_ERR(err);

		MemAllocate.allocationSize = MemRequirements.size;

		// Find a memory type matching the properties we need
		MemTypeIndex = Util::MemoryTypeFromProperties(Instance, MemRequirements.memoryTypeBits,
		                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | MemoryProperty);

		assert(MemTypeIndex != ~0U);

		// Allocate the memory
		err = vkAllocateMemory(*Instance.GetDevice(), &MemAllocate, nullptr, &mMemory);
		CHECK_ERR(err);

			// Map the memory to the buffer
		err = vkBindBufferMemory(*Instance.GetDevice(), mBuffer, mMemory, 0);
		CHECK_ERR(err);

		// Setup descriptor
		mDesc.buffer = mBuffer;
		mDesc.offset = 0;
		mDesc.range = Size;

		mSize = Size;
	}

	UniformBuffer::~UniformBuffer()
	{
	}

	void UniformBuffer::MapData(Vulkan::InstanceObject& Instance)
	{
		// Map this memory
		VkResult err;
		err = vkMapMemory(*Instance.GetDevice(), mMemory, 0, mSize, 0, &mData);
		CHECK_ERR(err);
	}

	void UniformBuffer::UnmapData(Vulkan::InstanceObject& Instance)
	{
		vkUnmapMemory(*Instance.GetDevice(), mMemory);
	}

}

