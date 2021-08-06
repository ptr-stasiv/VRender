#pragma once
#include "vrender.h"

#include "buffer.h"
#include "descriptor.h"

namespace vk
{
	//Dynamic use more buffer to properly update them in main loop
	enum class UboType
	{
		Static,
		Dynamic
	};

	class API UniformBuffer
	{
	private:
		std::vector<Buffer> Buffers;

		UboType Type;

		vk::VulkanApp* VulkanApp;
	public:
		void Setup(vk::VulkanApp& app, const UboType type, const size_t stride, const size_t elementsCount);

		//Use this only for dynamic buffer
		inline void Update(const size_t imageId, void* data, const size_t elementsCount)
		{
			if (imageId >= Buffers.size()
			    || Type == UboType::Static)
			{
				LOG("Invalid buffer id passed or you use static type buffer: %d", imageId)
				return;
			}

			Buffers[imageId].Update(data, elementsCount);
		}
		
		inline void Update(void* data, const size_t elementsCount)
		{
			for (auto& b : Buffers)
				b.Update(data, elementsCount);
		}

		inline void Cleanup()
		{
			for (auto& b : Buffers)
				b.Cleanup();
		}

		inline auto GetBufferInfos() const
		{
			std::vector<VkDescriptorBufferInfo> bufferInfos;

			for (auto& vbo : Buffers)
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = vbo.GetHandler();
				bufferInfo.offset = 0;
				bufferInfo.range = VK_WHOLE_SIZE;

				bufferInfos.push_back(bufferInfo);
			}

			return bufferInfos;
		}
	};

	class UboDescriptor
	{
	private:
		Descriptor DescriptorInfo;

		struct
		{
			std::vector<VkDescriptorSetLayoutBinding> LayoutBindInfos;
			std::vector<std::vector<VkDescriptorBufferInfo>> BufferInfos;
		} UboInfos;

		VulkanApp* App;
	public:
		void Create(VulkanApp& app, const VkDescriptorPool& descriptorPool);
		void Destroy();

		inline void LinkUBO(const UniformBuffer& ubo, const uint8_t bindId)
		{
			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = bindId;
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

			UboInfos.LayoutBindInfos.push_back({ layoutBinding });

			UboInfos.BufferInfos.push_back(ubo.GetBufferInfos());
		}

		inline Descriptor GetDescriptorInfo() const
		{
			return DescriptorInfo;
		}
	};
}