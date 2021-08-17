#include "helpers.h"

#include <fstream>

namespace vk
{
	std::optional<std::vector<char>> ReadShader(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			return std::nullopt;

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> bytecode(fileSize);

		file.seekg(0);
		file.read(bytecode.data(), fileSize);

		file.close();

		return bytecode;
	}

	int32_t FindMemoryType(const vk::VulkanApp& app, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(app.PhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
		{
			if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties
				&& typeFilter & (1 << i))
			{
				return i;
			}
		}

		return -1;
	}

	VkCommandBuffer BeginCommands(const VulkanApp& app, const VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(app.Device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void EndCommands(const VulkanApp& app, const VkCommandPool commandPool, const VkCommandBuffer commandBuffer, const VkQueue queue)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(app.Device, commandPool, 1, &commandBuffer);
	}

	void TransitionImageLayout(const VulkanApp& app, const VkImage image, const VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout)
	{
		auto commandBuffer = BeginCommands(app, app.CommandPoolGQ);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;


		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
			&& newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			     && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		EndCommands(app, app.CommandPoolGQ, commandBuffer, app.GraphicsQueue);
	}

	void CopyBufferToImage(const VulkanApp& app, const VkBuffer buffer, const VkImage image, const uint16_t width, const uint16_t height)
	{
		auto commandBuffer = BeginCommands(app, app.CommandPoolGQ);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		EndCommands(app, app.CommandPoolGQ, commandBuffer, app.GraphicsQueue);
	}

	std::optional<VkRenderPass> CreateRenderPass(const VulkanApp& app, const std::vector<VkAttachmentDescription>& attachments,
												 const std::vector<VkSubpassDescription>& subpasses,
												 const std::vector<VkSubpassDependency>& dependencies)
	{
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = subpasses.size();
		renderPassInfo.pSubpasses = subpasses.data();
		renderPassInfo.dependencyCount = dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();

		VkRenderPass renderPass;

		if (vkCreateRenderPass(app.Device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
			return std::nullopt;

		return renderPass;
	}

	std::optional<VkFramebuffer> CreateFramebuffer(const VulkanApp& app, const VkRenderPass& renderPass,
												   const std::vector<VkImageView>& attachments, 
												   const uint16_t width, const uint16_t height)
	{
		VkFramebufferCreateInfo fboInfo{};
		fboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fboInfo.renderPass = renderPass;
		fboInfo.attachmentCount = attachments.size();
		fboInfo.pAttachments = attachments.data();
		fboInfo.width = width;
		fboInfo.height = height;
		fboInfo.layers = 1;

		VkFramebuffer fbo;

		if (vkCreateFramebuffer(app.Device, &fboInfo, nullptr, &fbo) != VK_SUCCESS)
			return std::nullopt;

		return fbo;
	}

	std::optional<Pipeline> CreateGraphicsPipeline(const VulkanApp& app,
												   const VkRenderPass& renderPass,
												   const vk::Shader& shader,
												   const std::vector< VkDescriptorSetLayout>& layouts,
												   const VkPipelineInputAssemblyStateCreateInfo& inputAssembly,
												   const VkPipelineViewportStateCreateInfo& viewportState,
												   const VkPipelineRasterizationStateCreateInfo& rasterizer,
												   const VkPipelineMultisampleStateCreateInfo& multisample,
												   const VkPipelineColorBlendStateCreateInfo& colorBlending,
												   const VkPipelineDepthStencilStateCreateInfo& depthState)
	{
		VkPipelineLayout pipelineLayout;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = layouts.size();
		pipelineLayoutInfo.pSetLayouts = layouts.data();

		if (vkCreatePipelineLayout(app.Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
			return std::nullopt;


		VkPipeline pipeline;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shader.GetStages().size();
		pipelineInfo.pStages = shader.GetStages().data();
		pipelineInfo.pVertexInputState = &shader.GetInputState();
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisample;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &depthState;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(app.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
			return std::nullopt;

		return { { pipelineLayout, pipeline } };
	}
}