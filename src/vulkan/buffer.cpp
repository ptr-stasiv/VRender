#include "buffer.h"

#include "helpers.h"

namespace vk
{
	void Buffer::Setup(vk::VulkanApp& app, const VkBufferUsageFlags usageFlags, const size_t stride, const size_t elementsCount)
	{
		VulkanApp = &app;

		ElementsCount = elementsCount;
		Stride = stride;

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = Stride * ElementsCount;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(VulkanApp->Device, &bufferCreateInfo, nullptr, &BufferH) != VK_SUCCESS)
		{
			LOG("Buffer creation error!");
			return;
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(VulkanApp->Device, BufferH, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(*VulkanApp, memRequirements.memoryTypeBits, 
												   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
												   | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(VulkanApp->Device, &allocInfo, nullptr, &BufferMemory) != VK_SUCCESS)
		{
			LOG("Couldn't allocate buffer memory!");
			return;
		}

		vkBindBufferMemory(VulkanApp->Device, BufferH, BufferMemory, 0);
	}
}