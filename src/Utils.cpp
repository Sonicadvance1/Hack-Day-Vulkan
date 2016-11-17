#include "Vulkan.h"

namespace Vulkan
{
namespace Util
{
uint32_t MemoryTypeFromProperties(Vulkan::InstanceObject& Instance, uint32_t TypeBits,
                                  VkFlags RequirementsMask)
{
	const auto MemProps = Instance.GetMemProp();
	for (uint32_t i = 0; i < 32; ++i)
	{
		if (!!(TypeBits & (1 << i)))
		{
			// Does this memory property support our requirements?
			if ((MemProps->memoryTypes[i].propertyFlags & RequirementsMask) == RequirementsMask)
			{
				return i;
			}
		}
	}

	// Nothing found that matches the requirements
	return ~0U;
}
}
}
