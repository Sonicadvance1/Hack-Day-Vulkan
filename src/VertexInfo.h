#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Vulkan
{
class InstanceObject;

class VertexBuffer
{
public:
	VertexBuffer(Vulkan::InstanceObject& Instance,
	             const std::vector<float>& Vertices,
			 uint32_t BindingID,
	             uint32_t Stride,
			 VkBufferUsageFlagBits Usage);
	~VertexBuffer();

	static std::unique_ptr<VertexBuffer> Create(Vulkan::InstanceObject& Instance,
		const std::vector<float>& Vertices,
		uint32_t BindingID,
		uint32_t Stride,
		VkBufferUsageFlagBits Usage)
	{
		return std::make_unique<VertexBuffer>(Instance, Vertices, BindingID, Stride, Usage);
	}

	void AddAttribute(VkVertexInputAttributeDescription VIAttribute);

	// Device Objects
	VkBuffer* GetBuffer() { return &mBuffer; }
	VkDeviceMemory GetMemory() const { return mMemory; }

	VkPipelineVertexInputStateCreateInfo* GetVI() { return &mVI; }
	VkVertexInputAttributeDescription* GetAttributes() { return mVIAttributes; }

	// Information
	uint32_t GetBindingID() const { return mBindingID; }

private:
	VkBuffer mBuffer{};
	VkDeviceMemory mMemory{};

	static VkPipelineVertexInputStateCreateInfo mVI;
	VkVertexInputBindingDescription mVIBinding{};
	VkVertexInputAttributeDescription mVIAttributes[16]{};

	const uint32_t mBindingID;

	uint32_t mNumAttribs = 0;
};

class IndicesBuffer
{
public:
	IndicesBuffer(Vulkan::InstanceObject& Instance,
	              const std::vector<uint32_t>& Indices,
	              VkBufferUsageFlagBits Usage);
	~IndicesBuffer();

	static std::unique_ptr<IndicesBuffer> Create(Vulkan::InstanceObject& Instance,
		const std::vector<uint32_t>& Indices, VkBufferUsageFlagBits Usage)
	{
		return std::make_unique<IndicesBuffer>(Instance, Indices, Usage);
	}

	// Device Objects
	VkBuffer GetBuffer() const { return mBuffer; }
	VkDeviceMemory GetMemory() const { return mMemory; }

	// Information
	uint32_t GetCount() const { return mCount; }
private:
	VkBuffer mBuffer;
	VkDeviceMemory mMemory;
	uint32_t mCount;
};

class UniformBuffer
{
public:

	UniformBuffer(Vulkan::InstanceObject& Instance,
	              uint32_t Size, VkMemoryPropertyFlagBits MemoryProperty);
	~UniformBuffer();

	static std::unique_ptr<UniformBuffer> Create(Vulkan::InstanceObject& Instance,
		uint32_t Size, VkMemoryPropertyFlagBits MemoryProperty)
	{
		return std::make_unique<UniformBuffer>(Instance, Size, MemoryProperty);
	}

	// Device Objects
	VkBuffer GetBuffer() const { return mBuffer; }
	VkDeviceMemory GetMemory() const { return mMemory; }
	const VkDescriptorBufferInfo* GetDesc() const { return &mDesc; }

	// Information
	uint32_t GetSize() const { return mSize; }

	void MapData(Vulkan::InstanceObject& Instance);
	void UnmapData(Vulkan::InstanceObject& Instance);
	template<typename T>
	T* GetData() const { return static_cast<T*>(mData); }

private:
	VkBuffer mBuffer{};
	VkDeviceMemory mMemory{};
	VkDescriptorBufferInfo mDesc{};

	uint32_t mSize;

	void *mData;
};

}
