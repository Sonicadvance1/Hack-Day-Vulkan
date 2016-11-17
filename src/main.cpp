#include "Context.h"
#include "PNGLoader.h"
#include "Vulkan.h"

#include <assert.h>
#include <atomic>
#include <chrono>
#include <stdio.h>
#include <string.h>
#include <thread>

GLFWwindow* gWin;
int32_t gWidth, gHeight;

float zoom = -2.5f;
glm::vec3 rotation{};

const uint32_t VERTEX_BUFFER_BIND_ID = 0;

void GetInstanceInfo()
{
	std::vector<VkLayerProperties> Layers;
	std::vector<VkExtensionProperties> Extensions;
	Vulkan::GetInstanceValidationLayers(&Layers);
	Vulkan::GetInstanceExtensions(&Extensions);

	printf("Instance Information\n");

	printf("We have %zd validation layers\n", Layers.size());
	printf("----------------------\n");
	for (const auto& layer : Layers)
	{
		printf("Name: %s\n", layer.layerName);
		printf("Version: %d\n", layer.specVersion);
		printf("ImplVersion: %d\n", layer.implementationVersion);
		printf("Desc: %s\n", layer.description);
		printf("----------------------\n");
	}

	uint32_t RequiredExtensionCount;
	const char** RequiredExtensions;

	RequiredExtensions = Vulkan::GetRequiredExtensions(&RequiredExtensionCount);

	printf("Required Extensions\n");
	for (int i = 0; i < RequiredExtensionCount; ++i)
	{
		printf("%d: %s\n", i, RequiredExtensions[i]);
	}

	printf("We have %zd extensions\n", Extensions.size());
	printf("----------------------\n");

	for (const auto& extension : Extensions)
	{
		printf("Name: %s\n", extension.extensionName);
		printf("Version: %d\n", extension.specVersion);
		printf("----------------------\n");
	}
}

void GetDeviceInfo(Vulkan::InstanceObject& Instance)
{
	std::vector<VkLayerProperties> Layers;
	std::vector<VkExtensionProperties> Extensions;
	std::vector<VkQueueFamilyProperties> Queues;

	Vulkan::GetDeviceValidationLayers(Instance, &Layers);
	Vulkan::GetDeviceExtensions(Instance, &Extensions);
	Vulkan::GetDeviceQueueProperties(Instance, &Queues);

	printf("Device Information\n");

	// Physical device validation layers
	printf("We have %zd validation layers\n", Layers.size());
	printf("----------------------\n");
	for (const auto& layer : Layers)
	{
		printf("Name: %s\n", layer.layerName);
		printf("Version: %d\n", layer.specVersion);
		printf("ImplVersion: %d\n", layer.implementationVersion);
		printf("Desc: %s\n", layer.description);
		printf("----------------------\n");
	}

	// Physical device extensions
	printf("We have %zd extensions\n", Extensions.size());
	printf("----------------------\n");

	for (const auto& extension : Extensions)
	{
		printf("Name: %s\n", extension.extensionName);
		printf("Version: %d\n", extension.specVersion);
		printf("----------------------\n");
	}

	// Physical device properties
	Vulkan::GetDeviceProperties(Instance);
	VkPhysicalDeviceProperties* GPUProp = Instance.GetGPUProp();

	printf("Physical Device Properties\n");
	printf("----------------------\n");
	printf("Name: %s\n", GPUProp->deviceName);
	printf("API Version: %d.%d.%d\n",
			VK_VERSION_MAJOR(GPUProp->apiVersion),
			VK_VERSION_MINOR(GPUProp->apiVersion),
			VK_VERSION_PATCH(GPUProp->apiVersion));
	printf("Driver Verison: %d\n", GPUProp->driverVersion);
	printf("Vendor/Device ID: %d:%d\n", GPUProp->vendorID, GPUProp->deviceID);
	printf("Device Type: %d\n", GPUProp->deviceType);
	printf("----------------------\n");

	// Physical device queue properties
	printf("Physical Device Queue Properties\n");

	printf("We have %zd queue properties\n", Queues.size());
	printf("----------------------\n");

	const std::array<std::pair<VkQueueFlagBits, std::string>, 4> QueueFlags =
	{{
		{ VK_QUEUE_GRAPHICS_BIT, "Graphics" },
		{ VK_QUEUE_COMPUTE_BIT, "Compute" },
	    	{ VK_QUEUE_TRANSFER_BIT, "Transfer" },
	    	{ VK_QUEUE_SPARSE_BINDING_BIT, "Sparse" },
	}};
	for (const auto& queue : Queues)
	{
		printf("QueueFlags: 0x%x\n", queue.queueFlags);
		for (const auto& flag : QueueFlags)
		{
			if (!(flag.first & queue.queueFlags))
				continue;

			printf("\t%s\n", flag.second.c_str());
		}
		printf("QueueCount: %d\n", queue.queueCount);
		printf("Timestamp Valid bits: %d\n", queue.timestampValidBits);
		printf("----------------------\n");
	}
}

void SetImageLayout(Vulkan::InstanceObject& Instance, VkImage Image,
                    VkImageAspectFlags AspectMask,
			  VkImageLayout OldLayout,
			  VkImageLayout NewLayout)
{
	VkResult err;
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

	// Let's actually set the image layout now
	VkImageMemoryBarrier MemoryBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = OldLayout,
		.newLayout = NewLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = Image,
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

	Vulkan::SubmitSetupQueue(Instance);
}

void GenerateSwapChain(Vulkan::InstanceObject& Instance)
{
	std::vector<VkQueueFamilyProperties> Queues;
	Vulkan::GetDeviceQueueProperties(Instance, &Queues);

	Vulkan::CreateWindowSurface(Instance, gWin);

	uint32_t PresentGraphicsQueue = ~0U;
	for (uint32_t i = 0; i < Queues.size(); ++i)
	{
		if ((Queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
		    Vulkan::QueueSupportsPresent(Instance, i))
		{
			PresentGraphicsQueue = i;
			break;
		}
	}
	assert(PresentGraphicsQueue != ~0U);

	Instance.SetPresentQueueIndex(PresentGraphicsQueue);

	Vulkan::CreateDevice(Instance);

	std::vector<VkSurfaceFormatKHR> SurfaceFormats;

	Vulkan::GetSurfaceFormats(Instance, &SurfaceFormats);

	printf("We have %zd formats\n", SurfaceFormats.size());

	for (const auto& format : SurfaceFormats)
	{
		printf("Format has %d type, colorspace %d\n", format.format, format.colorSpace);
	}

	// For now let's just use format 0
	Instance.SetSurfaceFormat(SurfaceFormats[0].format);

	Vulkan::GetMemoryProperties(Instance);

	Vulkan::CreateCommandPool(Instance);

	Vulkan::CreateSwapChain(Instance);
}

void GetSurfaceCapabilities(Vulkan::InstanceObject& Instance)
{
	VkResult err;
	VkSurfaceCapabilitiesKHR SurfaceCaps;
	err = Instance.GetPhysicalDeviceSurfaceCapabilitiesKHR(Instance.GetGPU(), *Instance.GetSurface(), &SurfaceCaps);
	CHECK_ERR(err);

	printf("Surface Capabilities\n");
	printf("----------------------\n");
	printf("Min/Max Image count: %d/%d\n", SurfaceCaps.minImageCount, SurfaceCaps.maxImageCount);
	printf("Current Extent: %d/%d\n", SurfaceCaps.currentExtent.width, SurfaceCaps.currentExtent.height);
	printf("Min Extent: %d/%d\n", SurfaceCaps.minImageExtent.width, SurfaceCaps.minImageExtent.height);
	printf("Max Extent: %d/%d\n", SurfaceCaps.maxImageExtent.width, SurfaceCaps.maxImageExtent.height);
	printf("Max image layers: %d\n", SurfaceCaps.maxImageArrayLayers);
}

void BuildCommandList(Vulkan::InstanceObject& Instance)
{
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

	VkClearValue ClearValue[2]{};
	ClearValue[0].color = {{0.2, 0.2, 0.2, 0.2}};
	ClearValue[1].depthStencil = {1.0f, 0};

	const VkRenderPassBeginInfo RenderPassBeginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = Instance.mRenderPass,
		.framebuffer = Instance.mFramebuffers[Instance.mCurrentSwapBuffer],
		.renderArea =
		{
			.offset =
				{
					.x = 0,
					.y = 0,
				},
			.extent =
				{
					.width = (uint32_t)gWidth,
					.height = (uint32_t)gHeight,
				},
		},
		.clearValueCount = 2,
		.pClearValues = ClearValue,
	};

	VkResult err;

	err = vkBeginCommandBuffer(Instance.mDrawCommand, &CommandBufferInfo);
	CHECK_ERR(err);

	vkCmdBeginRenderPass(Instance.mDrawCommand, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(Instance.mDrawCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, Instance.mPipeline);
	vkCmdBindDescriptorSets(Instance.mDrawCommand, VK_PIPELINE_BIND_POINT_GRAPHICS, Instance.mPipelineLayout,
	                        0, 1, &Instance.mDescriptorSet, 0, nullptr);

	VkViewport VP{};
	VP.height = (float)gHeight;
	VP.width = (float)gWidth;
	VP.minDepth = 0.0f;
	VP.maxDepth = 1.0f;
	vkCmdSetViewport(Instance.mDrawCommand, 0, 1, &VP);

	VkRect2D Scissor{};

	Scissor.extent.width = gWidth;
	Scissor.extent.height = gHeight;
	Scissor.offset.x = 0;
	Scissor.offset.y = 0;
	vkCmdSetScissor(Instance.mDrawCommand, 0, 1, &Scissor);

	VkDeviceSize Offsets{};
	vkCmdBindVertexBuffers(Instance.mDrawCommand, VERTEX_BUFFER_BIND_ID, 1, Instance.mVertices->GetBuffer(), &Offsets);

#if 0
	// Bind triangle index buffer
	vkCmdBindIndexBuffer(Instance.mDrawCommand, Instance.mIndices->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

	// Draw indexed triangle
	vkCmdDrawIndexed(Instance.mDrawCommand, Instance.mIndices->GetCount(), 1, 0, 0, 1);
#else
	vkCmdDraw(Instance.mDrawCommand, Instance.mVerticeCount, 1, 0, 0);
#endif

	vkCmdEndRenderPass(Instance.mDrawCommand);

	VkImageMemoryBarrier PrePresentBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = Instance.mSwapChainBuffers[Instance.mCurrentSwapBuffer].mImage,
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
	};

	vkCmdPipelineBarrier(Instance.mDrawCommand,
	                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				   0, 0, nullptr, 0, nullptr, 1, &PrePresentBarrier);
	err = vkEndCommandBuffer(Instance.mDrawCommand);
	CHECK_ERR(err);
}

void RenderVulkan(Vulkan::InstanceObject& Instance)
{
	VkResult err;
	VkSemaphore PresentCompleteSema;
	VkSemaphoreCreateInfo PresentCompleteSemaInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
	};

	err = vkCreateSemaphore(*Instance.GetDevice(), &PresentCompleteSemaInfo, nullptr, &PresentCompleteSema);
	CHECK_ERR(err);

	err = Instance.AcquireNextImageKHR(*Instance.GetDevice(), Instance.mSwapChain, UINT64_MAX,
	                                   PresentCompleteSema, nullptr, &Instance.mCurrentSwapBuffer);

	CHECK_ERR(err);

	SetImageLayout(Instance, Instance.mSwapChainBuffers[Instance.mCurrentSwapBuffer].mImage,
	               VK_IMAGE_ASPECT_COLOR_BIT,
	               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	BuildCommandList(Instance);

	// Submit a queue
	VkFence NullFence = VK_NULL_HANDLE;
	VkPipelineStageFlags PipeStageFlag = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo SubmitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &PresentCompleteSema,
		.pWaitDstStageMask = &PipeStageFlag,
		.commandBufferCount = 1,
		.pCommandBuffers = &Instance.mDrawCommand,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr,
	};

	err = vkQueueSubmit(*Instance.GetQueue(), 1, &SubmitInfo, NullFence);
	CHECK_ERR(err);

	// Let's do a present!
	const VkPresentInfoKHR PresentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 0, // XXX: Not use 0?
		.pWaitSemaphores = nullptr,
		.swapchainCount = 1,
		.pSwapchains = &Instance.mSwapChain,
		.pImageIndices = &Instance.mCurrentSwapBuffer,
		.pResults = nullptr, // XXX: What results?
	};

	err = Instance.QueuePresentKHR(*Instance.GetQueue(), &PresentInfo);

	CHECK_ERR(err);

	err = vkQueueWaitIdle(*Instance.GetQueue());
	CHECK_ERR(err);

	vkDestroySemaphore(*Instance.GetDevice(), PresentCompleteSema, nullptr);
}

void GenerateRenderPass(Vulkan::InstanceObject& Instance)
{
	const VkAttachmentDescription Attachment =
	{
		.flags = 0,
		.format = Instance.GetSurfaceFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	const VkAttachmentReference ColorReference =
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	const VkSubpassDescription SubPass =
	{
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &ColorReference,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr,
	};

	const VkRenderPassCreateInfo RenderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &Attachment,
		.subpassCount = 1,
		.pSubpasses = &SubPass,
		.dependencyCount = 0,
		.pDependencies = nullptr,
	};

	VkResult err;
	err = vkCreateRenderPass(*Instance.GetDevice(), &RenderPassInfo, nullptr, &Instance.mRenderPass);
	CHECK_ERR(err);
}

void GenerateFramebuffers(Vulkan::InstanceObject& Instance)
{
	VkImageView Attachment[2];
	Attachment[1] = Instance.mDepth->GetView();

	const VkFramebufferCreateInfo FramebufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = Instance.mRenderPass,
		.attachmentCount = 1,
		.pAttachments = Attachment,
		.width = (uint32_t)gWidth,
		.height = (uint32_t)gHeight,
		.layers = 1,
	};

	VkResult err;

	Instance.mFramebuffers.resize(Instance.mSwapChainBuffers.size());

	for (int i = 0; i < Instance.mFramebuffers.size(); ++i)
	{
		Attachment[0] = Instance.mSwapChainBuffers[i].mView;
		err = vkCreateFramebuffer(*Instance.GetDevice(), &FramebufferInfo, nullptr, &Instance.mFramebuffers[i]);
		CHECK_ERR(err);
	}
}

VkShaderModule PrepareVSModule(Vulkan::InstanceObject& Instance)
{
	const char vss[] =
		"#version 450 core\n"
		"layout(location = 0) in vec3 aVertex;\n"
		"layout(location = 1) in vec4 aColor;\n"
		"layout(std140, binding = 0) uniform Block\n"
		"{\n"
		"	mat4 projectionMatrix;\n"
		"	mat4 modelMatrix;\n"
		"	mat4 viewMatrix;\n"
		"};\n"
		"out vec4 vColor;\n"
		"void main()\n"
		"{\n"
		"    vColor = aColor;\n"
		"    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aVertex, 1.0);\n"
		"}\n";

	VkResult err;
	VkShaderModule Module;
	VkShaderModuleCreateInfo ModuleCreateInfo;
	ModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ModuleCreateInfo.pNext = nullptr;
	ModuleCreateInfo.flags = 0;
	ModuleCreateInfo.codeSize = sizeof(vss);
	ModuleCreateInfo.pCode = (const uint32_t*)vss;

	err = vkCreateShaderModule(*Instance.GetDevice(), &ModuleCreateInfo, nullptr, &Module);
	CHECK_ERR(err);

	return Module;
}

VkShaderModule PrepareFSModule(Vulkan::InstanceObject& Instance)
{
	const char fss[] =
		"#version 450 core\n"
		"in vec4 vColor;\n"
		"out vec4 ocol;\n"
		"layout(binding = 0) uniform sampler2D mySampler;\n"
		"void main()\n"
		"{\n"
		"	ocol = vColor;\n"
//		"	ocol = texture(mySampler, vec2(1.0, 1.0));\n"
		"}\n";

	VkResult err;
	VkShaderModule Module;
	VkShaderModuleCreateInfo ModuleCreateInfo;
	ModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ModuleCreateInfo.pNext = nullptr;
	ModuleCreateInfo.flags = 0;
	ModuleCreateInfo.codeSize = sizeof(fss);
	ModuleCreateInfo.pCode = (const uint32_t*)fss;

	err = vkCreateShaderModule(*Instance.GetDevice(), &ModuleCreateInfo, nullptr, &Module);
	CHECK_ERR(err);

	return Module;
}

void GeneratePipeline(Vulkan::InstanceObject& Instance)
{
	VkGraphicsPipelineCreateInfo Pipeline{};
	VkPipelineCacheCreateInfo PipelineCache{};

	VkPipelineInputAssemblyStateCreateInfo ia{};
	VkPipelineRasterizationStateCreateInfo rs{};
	VkPipelineColorBlendStateCreateInfo cb{};
	VkPipelineDepthStencilStateCreateInfo ds{};
	VkPipelineViewportStateCreateInfo vp{};
	VkPipelineMultisampleStateCreateInfo ms{};
	VkDynamicState DynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE]{};
	VkPipelineDynamicStateCreateInfo DynamicState{};

	VkResult err;

	// Dynamic State
	DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	DynamicState.pDynamicStates = DynamicStateEnables;

	// Pipeline
	Pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	Pipeline.layout = Instance.mPipelineLayout;

	// Input assembly state
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	// Rasterization state
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_FRONT_BIT;
	rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;

	// Color Blend state
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState AttachState;
	AttachState.colorWriteMask = 0xF;
	AttachState.blendEnable = VK_FALSE;
	cb.attachmentCount = 1;
	cb.pAttachments = &AttachState;

	// Depth/Stencil state
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.depthTestEnable = VK_TRUE;
	ds.depthWriteEnable = VK_TRUE;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
	ds.stencilTestEnable = VK_FALSE;
	ds.front = ds.back;

	// Multisample state
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pSampleMask = nullptr;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Two shader stages
	Pipeline.stageCount = 2;
	VkPipelineShaderStageCreateInfo ShaderStage[2]{};

	// VKShader Module
	ShaderStage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	ShaderStage[0].module = PrepareVSModule(Instance);
	ShaderStage[0].pName = "main";

	ShaderStage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	ShaderStage[1].module = PrepareFSModule(Instance);
	ShaderStage[1].pName = "main";

	// Set all the Pipeline state
	Pipeline.pVertexInputState = Instance.mVertices->GetVI();
	Pipeline.pInputAssemblyState = &ia;
	Pipeline.pRasterizationState = &rs;
	Pipeline.pColorBlendState = &cb;
	Pipeline.pMultisampleState = &ms;
	Pipeline.pViewportState = &vp;
	Pipeline.pDepthStencilState = &ds;
	Pipeline.pStages = ShaderStage;
	Pipeline.renderPass = Instance.mRenderPass;
	Pipeline.pDynamicState = &DynamicState;

	// Pipeline cache
	PipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	err = vkCreatePipelineCache(*Instance.GetDevice(), &PipelineCache, nullptr, &Instance.mPipelineCache);
	CHECK_ERR(err);

	err = vkCreateGraphicsPipelines(*Instance.GetDevice(), Instance.mPipelineCache, 1, &Pipeline, nullptr, &Instance.mPipeline);
	CHECK_ERR(err);

	vkDestroyPipelineCache(*Instance.GetDevice(), Instance.mPipelineCache, nullptr);
}

void GenerateDescriptorLayout(Vulkan::InstanceObject& Instance)
{
	const VkDescriptorSetLayoutBinding LayoutBinding[] =
	{
		{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = nullptr,
		},
		{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr,
		},
	};

	const VkDescriptorSetLayoutCreateInfo DescriptorLayout =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = 2,
		.pBindings = LayoutBinding,
	};

	VkResult err;
	err = vkCreateDescriptorSetLayout(*Instance.GetDevice(), &DescriptorLayout, nullptr, &Instance.mDescriptorLayout);
	CHECK_ERR(err);

	const VkPipelineLayoutCreateInfo PipelineLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &Instance.mDescriptorLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	err = vkCreatePipelineLayout(*Instance.GetDevice(), &PipelineLayoutInfo, nullptr, &Instance.mPipelineLayout);
	CHECK_ERR(err);
}

void GenerateDescriptorPool(Vulkan::InstanceObject& Instance)
{
	const VkDescriptorPoolSize TypeCount =
	{
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
	};

	// Pool size of 0 causes vkCreateDescriptorPool to crash
	const VkDescriptorPoolCreateInfo DescriptorPool =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &TypeCount,
	};

	VkResult err;

	err = vkCreateDescriptorPool(*Instance.GetDevice(), &DescriptorPool, nullptr, &Instance.mDescriptorPool);
	CHECK_ERR(err);
}

void GenerateDescriptorSet(Vulkan::InstanceObject& Instance)
{
	VkResult err;
	VkDescriptorSetAllocateInfo AllocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = Instance.mDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &Instance.mDescriptorLayout,
	};

	err = vkAllocateDescriptorSets(*Instance.GetDevice(), &AllocInfo, &Instance.mDescriptorSet);
	CHECK_ERR(err);

	// Set up samplers
	VkDescriptorImageInfo TextureInfo =
	{
		.sampler = Instance.mSampler->GetSampler(),
		.imageView = Instance.mSampler->GetTexture()->GetView(),
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	VkWriteDescriptorSet Write[2] =
	{
		{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = Instance.mDescriptorSet,
		// Binds UBO to binding point 0
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = Instance.mUBO->GetDesc(),
		.pTexelBufferView = nullptr,
		},

		{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = Instance.mDescriptorSet,
		// Binds Sampler to binding point 0
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &TextureInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr,
		},

	};

	vkUpdateDescriptorSets(*Instance.GetDevice(), 2, Write, 0, nullptr);
}

void UpdateUniformBuffer(Vulkan::InstanceObject& Instance)
{
	Instance.mUBOData.projectionMatrix = glm::perspective(glm::radians(60.0f), (float)gWidth / (float)gHeight, 0.1f, 256.0f);

	Instance.mUBOData.viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

	Instance.mUBOData.modelMatrix = glm::mat4();
	Instance.mUBOData.modelMatrix = glm::rotate(Instance.mUBOData.modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	Instance.mUBOData.modelMatrix = glm::rotate(Instance.mUBOData.modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	Instance.mUBOData.modelMatrix = glm::rotate(Instance.mUBOData.modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	size_t Size = sizeof(Instance.mUBOData);

	Instance.mUBO->MapData(Instance);
	uint8_t* Data = Instance.mUBO->GetData<uint8_t>();
	memcpy(Data, &Instance.mUBOData, Size);
	Instance.mUBO->UnmapData(Instance);
}

void GenerateTexture(Vulkan::InstanceObject& Instance)
{
	PNGLoader Png("../Data/Texture.png");

	VkExtent2D Dim { Png.GetWidth(), Png.GetHeight() };

	// Create our staging buffer
	std::unique_ptr<Vulkan::Texture2D> StagingTexture = Vulkan::Texture2D::CreateHost(Instance, Dim, 1, 1,
		VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	// Create our GPU side buffer
	std::unique_ptr<Vulkan::Texture2D> Texture = Vulkan::Texture2D::CreateGPU(Instance, Dim, 1, 1,
		VK_FORMAT_B8G8R8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	VkImageLayout DstLayout, SrcLayout;

	DstLayout = Texture->GetLayout();
	SrcLayout = StagingTexture->GetLayout();

	SetImageLayout(Instance, StagingTexture->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, SrcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	SetImageLayout(Instance, Texture->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, DstLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Copy the texture from the PNG
	StagingTexture->CopyToTexture(Instance, &Png);

	// Copy the texture from the staging buffer
	Texture->CopyFromTexture(Instance, StagingTexture.get());

	SetImageLayout(Instance, Texture->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DstLayout);

	// Create sampler
	Instance.mSampler = std::make_unique<Vulkan::Sampler>(Instance, std::move(Texture));

	Vulkan::SubmitSetupQueue(Instance);
}

void GenerateUniformBuffer(Vulkan::InstanceObject& Instance)
{
	size_t Size = sizeof(Instance.mUBOData);
	Instance.mUBO = Vulkan::UniformBuffer::Create(Instance, Size, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	UpdateUniformBuffer(Instance);
}

void GenerateVertices(Vulkan::InstanceObject& Instance)
{
	const uint32_t STRIDE = (3 + 4) * sizeof(float);
	const float w = 1;
	const float h = 1;
	const float d = 1;
	const std::vector<float> VertexBuffer =
	{
		// Position
		// Color
		// Front Face
		-1.0f, -1.0, 0.0,
		1.0f, 0.0f, 0.0f, 1.0f,

		-1.0f, 1.0, 0.0,
		0.0f, 1.0f, 0.0f, 1.0f,

		1.0f, -1.0, 0.0,
		0.0f, 0.0f, 1.0f, 1.0f,

		1.0f, 1.0, 0.0,
		1.0f, 1.0f, 1.0f, 1.0f,

	};

	Instance.mVertices = Vulkan::VertexBuffer::Create(Instance, VertexBuffer, VERTEX_BUFFER_BIND_ID, STRIDE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	Instance.mVerticeCount = VertexBuffer.size() / (3 + 4);

	// Setup vertex attributes
	Instance.mVertices->AddAttribute({
		.location = 0,
		.binding = Instance.mVertices->GetBindingID(),
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = 0});

	Instance.mVertices->AddAttribute({
		.location = 1,
		.binding = Instance.mVertices->GetBindingID(),
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		.offset = sizeof(float) * 3});

	// Indices
	const std::vector<uint32_t> IndicesBuffer =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
	};

	Instance.mIndices = Vulkan::IndicesBuffer::Create(Instance, IndicesBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void GenerateDepth(Vulkan::InstanceObject& Instance)
{
	const VkFormat DepthFormat = VK_FORMAT_D16_UNORM;

	printf("Generating an extent with dim %dx%d\n", gWidth, gHeight);

	VkExtent2D Dim { (uint32_t)gWidth, (uint32_t)gHeight };
	Instance.mDepth = Vulkan::Texture2D::CreateGPU(Instance, Dim,
		1, 1, DepthFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	Instance.mDepth->TransitionImageFormat(Instance, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void DoVulkanThings()
{
	GetInstanceInfo();
	Vulkan::InstanceObject Instance(true);

	printf("We have %d GPUs\n", Vulkan::GetGPUCount(Instance));
	Vulkan::UseGPU(Instance, 0); // Just use the first one

	//GetDeviceInfo(Instance);

	GenerateSwapChain(Instance);

	//GetSurfaceCapabilities(Instance);
	GenerateDepth(Instance);
	GenerateTexture(Instance);
	GenerateUniformBuffer(Instance);
	GenerateVertices(Instance);
	GenerateDescriptorLayout(Instance);
	GenerateRenderPass(Instance);
	GeneratePipeline(Instance);
	GenerateDescriptorPool(Instance);
	GenerateDescriptorSet(Instance);
	GenerateFramebuffers(Instance);

	// Run loop
	uint32_t iter = 0;
	auto start = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(gWin))
	{
		glfwPollEvents();
		RenderVulkan(Instance);
		vkDeviceWaitIdle(*Instance.GetDevice());

		UpdateUniformBuffer(Instance);
//		rotation.y += 0.01f;
		auto end = std::chrono::high_resolution_clock::now();
		auto diff = end - start;
		++iter;
		if (diff >= std::chrono::seconds(1))
		{
			start = std::chrono::high_resolution_clock::now();
			printf("%d loops in 1s\n", iter);
			iter = 0;
		}
	}
}

std::atomic<bool> mResized{false};
static void ResizeCallback(GLFWwindow* window, int width, int height)
{
	gWidth = width;
	gHeight = height;
	if (gWidth && gHeight)
		mResized = true;
}

int main()
{
	if (!Context::Init())
		return -1;

	gWidth = 640;
	gHeight = 480;
	gWin = Context::CreateWindow(gWidth, gHeight, "VulkanTest");
	glfwSetFramebufferSizeCallback(gWin, ResizeCallback);

//	while (!mResized.load()) { glfwPollEvents(); }
	DoVulkanThings();

	Context::DestroyWindow(gWin);
	Context::Shutdown();
	return 0;
}
