#include "texture.h"

#include "vulkan/buffer.h"
#include "vulkan/helpers.h"

namespace vk
{
	bool Texture::Setup(vk::VulkanApp& app, const uint16_t width, const uint16_t height,
						const TextureImageInfo& imageInfo, const TextureParams& params,
						const uint16_t depth, const uint16_t layersCount, const uint8_t mipLevels)
	{ 
		App = &app;

		if (!Image.Setup(app, imageInfo.Type, imageInfo.ViewType, imageInfo.Format,
						 imageInfo.UsageFlags, imageInfo.ViewAspect,
						 width, height, depth, layersCount, imageInfo.CreateFlags, imageInfo.Channels, mipLevels))
		{
			return false;
		}

		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = params.MagFilter;
		samplerCreateInfo.minFilter = params.MinFilter;
		samplerCreateInfo.addressModeU = params.AddressModeU;
		samplerCreateInfo.addressModeV = params.AddressModeV;
		samplerCreateInfo.addressModeW = params.AddressModeV;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		samplerCreateInfo.maxAnisotropy = app.DeviceProperties.limits.maxSamplerAnisotropy;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;

		if (vkCreateSampler(app.Device, &samplerCreateInfo, nullptr, &Sampler) != VK_SUCCESS)
			return false;


		VkDescriptorImageInfo info{};
		info.imageLayout = imageInfo.Layout;
		info.imageView = Image.GetViewHandler();
		info.sampler = Sampler;

		Info = info;
		ImageInfo = imageInfo;

		return true;
	}

	void Texture::Update(void* data, const size_t pixelStride)
	{
		Buffer buffer;
		buffer.Setup(*App, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, pixelStride, Image.GetWidth() * Image.GetHeight());

		buffer.Update(data, Image.GetWidth() * Image.GetHeight());

		layout::SetImageLayoutFromUndefinedToTransfer(*App, App->GraphicsQueue, App->CommandPoolGQ, Image.GetHandler());
		CopyBufferToImage(*App, buffer.GetHandler(), Image.GetHandler(), Image.GetWidth(), Image.GetHeight());

		buffer.Cleanup();
	}

	void TextureDescriptor::Create(vk::VulkanApp& app, DescriptorPoolManager& pm, const VkDescriptorType type)
	{
		App = &app;

		for (auto& lb : ImageInfos.LayoutBindInfos)
			lb.descriptorType = type;

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = ImageInfos.LayoutBindInfos.size();
		layoutCreateInfo.pBindings = ImageInfos.LayoutBindInfos.data();

		auto res = vkCreateDescriptorSetLayout(app.Device, &layoutCreateInfo, nullptr, &DescriptorInfo.DescriptorSetLayout);
		ASSERT(res == VK_SUCCESS, "Couldn't create descriptor set layout!");


		std::vector<VkDescriptorSetLayout> descriptorLayoutsCopies(1, DescriptorInfo.DescriptorSetLayout);
			

		DescriptorInfo.DescriptorSets = pm.GetAllocatedSets(ImageInfos.LayoutBindInfos[0].descriptorType, descriptorLayoutsCopies);

		for (size_t j = 0; j < ImageInfos.ImageInfos.size(); ++j)
		{
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = DescriptorInfo.DescriptorSets[0];
			descriptorWrite.dstBinding = ImageInfos.LayoutBindInfos[j].binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = type;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pImageInfo = &ImageInfos.ImageInfos[j];

			vkUpdateDescriptorSets(app.Device, 1, &descriptorWrite, 0, nullptr);
		}
	}
}