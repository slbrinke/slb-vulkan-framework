#include "Image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Image::Image(uint32_t width, uint32_t height, VkImageUsageFlags usage)
: m_width(width), m_height(height), m_usage(usage) {

}

Image::Image(std::shared_ptr<Context> &context, const std::string &fileName) {
    loadTexture(context, fileName);
}

Image::~Image() {

}

VkFormat Image::getFormat() {
    return m_format;
}

VkImageLayout Image::getLayout() {
    return m_layout;
}

VkImageLayout Image::getInitialLayout() {
    return m_initial;
}

VkImageLayout Image::getFinalLayout() {
    return m_final;
}

VkImageView Image::getView(uint32_t frame) {
    if(m_views.empty()) {
        throw std::runtime_error("IMAGE ERROR: Image has no view");
    }
    return m_views[glm::min(frame, (uint32_t)(m_views.size()-1))];
}

bool Image::hasHandle() {
    return m_handles.size() > 0;
}

bool Image::usesMultisampling() {
    return m_useMultisampling;
}

void Image::setNumLayers(uint32_t numLayers) {
    m_numLayers = numLayers;
}

void Image::setFormat(VkFormat format) {
    m_format = format;
}

void Image::setAspect(VkImageAspectFlags aspect) {
    m_aspect = aspect;
}

void Image::setLayout(VkImageLayout layout) {
    m_layout = layout;
}

void Image::setInitialLayout(VkImageLayout initial) {
    m_initial = initial;
}

void Image::setFinalLayout(VkImageLayout final) {
    m_final = final;
}

void Image::addUsage(VkImageUsageFlags usage) {
    m_usage |= usage;
}

void Image::enableMultisampling() {
    m_useMultisampling = true;
    m_usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
}

void Image::createAndAllocate(std::shared_ptr<Context> &context, uint32_t numFrames) {
    if(m_handles.size() > 0) {
        throw std::runtime_error("IMAGE ERROR: Image and memory has already been created");
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = m_numLayers;
    imageInfo.format = m_format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = m_usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = m_useMultisampling ? context->getMaxSamples() : VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    m_handles.resize(numFrames);
    m_memory.resize(numFrames);
    for(uint32_t f=0; f<numFrames; f++) {
        if(vkCreateImage(context->getDevice(), &imageInfo, nullptr, &m_handles[f]) != VK_SUCCESS) {
            throw std::runtime_error("IMAGE ERROR: Could not create image.");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(context->getDevice(), m_handles[f], &memoryRequirements);
        allocInfo.allocationSize = memoryRequirements.size;
        allocInfo.memoryTypeIndex = context->findMemoryType(memoryRequirements.memoryTypeBits, m_properties);

        if(vkAllocateMemory(context->getDevice(), &allocInfo, nullptr, &m_memory[f]) != VK_SUCCESS) {
            throw std::runtime_error("CONTEXT ERROR: Could not allocate image memory.");
        }

        vkBindImageMemory(context->getDevice(), m_handles[f], m_memory[f], 0);
    }
}

void Image::useSwapChain(std::shared_ptr<Context> &context, VkSwapchainKHR swapChain) {
    uint32_t numSwapChainImages;
    vkGetSwapchainImagesKHR(context->getDevice(), swapChain, &numSwapChainImages, nullptr);
    m_handles.resize(numSwapChainImages);
    vkGetSwapchainImagesKHR(context->getDevice(), swapChain, &numSwapChainImages, m_handles.data());
}

void Image::createViews(std::shared_ptr<Context> &context) {
    if(m_handles.empty()) {
        throw std::runtime_error("IMAGE ERROR: There is no image to create a view for");
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = m_numLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = m_format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = m_aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_numLayers;

    m_views.resize(m_handles.size());
    for(uint32_t v=0; v<m_handles.size(); v++) {
        viewInfo.image = m_handles[v];

        if(vkCreateImageView(context->getDevice(), &viewInfo, nullptr, &m_views[v]) != VK_SUCCESS) {
            throw std::runtime_error("TEXTURE ERROR: Could not create image view.");
        }
    }
}

void Image::transitionLayout(std::shared_ptr<Context> &context, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = context->startSingleCommand();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = m_aspect; //VK_IMAGE_ASPECT_COLOR_BIT
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_numLayers;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
            throw std::invalid_argument("CONTEXT ERROR: Unsupported layout transition.");
    }

    for(uint32_t f=0; f<m_handles.size(); f++) {
        barrier.image = m_handles[f];

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    context->endSingleCommand(commandBuffer);
}

void Image::copyBuffer(std::shared_ptr<Context> &context, VkBuffer buffer) {
    VkCommandBuffer commandBuffer = context->startSingleCommand();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = m_aspect;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = m_numLayers;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_width, m_height, 1};

    for(uint32_t f=0; f<m_handles.size(); f++) {
        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            m_handles[f],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
    }

    context->endSingleCommand(commandBuffer);
}

void Image::loadTexture(std::shared_ptr<Context> &context, const std::string &fileName) {
    //load file contents
    int width, height, numChannels;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* pixels = stbi_load(("../resources/textures/" + fileName).c_str(), &width, &height, &numChannels, STBI_rgb_alpha);
    if(!pixels) {
        throw std::runtime_error("IMAGE ERROR: Could not load file " + fileName);
    }

    //image settings
    m_width = static_cast<uint32_t>(width);
    m_height = static_cast<uint32_t>(height);
    m_format = VK_FORMAT_R8G8B8A8_SRGB;
    m_aspect = VK_IMAGE_ASPECT_COLOR_BIT;

    //write image content to buffer first
    VkDeviceSize imageSize = width * height * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    context->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(context->getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(context->getDevice(), stagingBufferMemory);
    stbi_image_free(pixels);

    //copy buffer to the final image
    m_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    createAndAllocate(context);
    transitionLayout(context, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBuffer(context, stagingBuffer);
    transitionLayout(context, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    createViews(context);

    vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(context->getDevice(), stagingBufferMemory, nullptr);
}

void Image::cleanUp(std::shared_ptr<Context> &context) {
    for(uint32_t m=0; m<m_memory.size(); m++) {
        vkDestroyImage(context->getDevice(), m_handles[m], nullptr);
        vkFreeMemory(context->getDevice(), m_memory[m], nullptr);
    }
    for(uint32_t v=0; v<m_views.size(); v++) {
        vkDestroyImageView(context->getDevice(), m_views[v], nullptr);
    }
}