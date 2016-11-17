#pragma once
#include "Vulkan.h"

namespace Vulkan
{
namespace Util
{
uint32_t MemoryTypeFromProperties(Vulkan::InstanceObject& Instance, uint32_t TypeBits,
                                  VkFlags RequirementsMask);
}
}
