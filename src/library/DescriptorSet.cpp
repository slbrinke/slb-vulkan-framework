#include "DescriptorSet.h"

DescriptorSet::DescriptorSet(std::shared_ptr<Context> &context, uint32_t numFramesInFlight)
: m_context(context), m_numFramesInFlight(numFramesInFlight) {

}

DescriptorSet::~DescriptorSet() {
    m_context = nullptr;
}

VkDescriptorSetLayout DescriptorSet::getLayout() {
    return m_layout;
}

VkDescriptorSet DescriptorSet::getSet(uint32_t frameIndex) {
    return m_sets[frameIndex];
}

void DescriptorSet::addBuffer(std::string name, VkDescriptorType descriptorType, VkDeviceSize bufferSize, bool doubleBinding, const void *data) {
    m_descriptors.resize(m_numDescriptors + 1);
    auto &descriptor = m_descriptors[m_numDescriptors];

    uint32_t offset = 0;
    if(m_numDescriptors > 0) {
        offset = m_descriptors[m_numDescriptors - 1].firstBinding + m_descriptors[m_numDescriptors - 1].numBindings;
    }
    descriptor.firstBinding = offset;
    descriptor.numBindings = 1;
    if(doubleBinding) {
        descriptor.numBindings++;
    }

    descriptor.name = name;
    descriptor.type = descriptorType;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if(descriptor.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
        usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    //create buffers
    descriptor.bufferSize = bufferSize;
    descriptor.buffers.resize(m_numFramesInFlight);
    descriptor.memory.resize(m_numFramesInFlight);
    if(descriptor.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
        descriptor.buffersMapped.resize(m_numFramesInFlight);
    }
    for(uint32_t frame=0; frame<m_numFramesInFlight; frame++) {
        m_context->createBuffer(
            descriptor.bufferSize, usage, properties,
            descriptor.buffers[frame], descriptor.memory[frame]);
        if(descriptor.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            vkMapMemory(
                m_context->getDevice(), descriptor.memory[frame],
                0, descriptor.bufferSize, 0,
                &descriptor.buffersMapped[frame]);
        }
    }

    //copy data to buffers if provided
    if(descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER && data != nullptr) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        m_context->createBuffer(descriptor.bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        void* bufferData;
        vkMapMemory(m_context->getDevice(), stagingBufferMemory, 0, descriptor.bufferSize, 0, &bufferData);
        memcpy(bufferData, data, (size_t)descriptor.bufferSize);
        vkUnmapMemory(m_context->getDevice(), stagingBufferMemory);

        for(uint32_t f=0; f<m_numFramesInFlight; f++) {
            m_context->copyBuffer(stagingBuffer, descriptor.buffers[f], descriptor.bufferSize);
        }

        vkDestroyBuffer(m_context->getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_context->getDevice(), stagingBufferMemory, nullptr);
    }

    m_numBufferBindings += descriptor.numBindings;
    m_numDescriptors++;
}

void DescriptorSet::addImage(VkDescriptorType descriptorType, VkImageView imageView) {
    m_descriptors.resize(m_numDescriptors + 1);
    auto &descriptor = m_descriptors[m_numDescriptors];

    uint32_t offset = 0;
    if(m_numDescriptors > 0) {
        offset = m_descriptors[m_numDescriptors - 1].firstBinding + m_descriptors[m_numDescriptors - 1].numBindings;
    }
    descriptor.firstBinding = offset;
    descriptor.numBindings = 1;

    descriptor.type = descriptorType;
    descriptor.numImages = 1;
    descriptor.imageViews.emplace_back(imageView);

    m_numImageBindings++;
    m_numImages++;
    m_numDescriptors++;

    if(descriptor.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && m_imageSampler == VK_NULL_HANDLE) {
        //throw std::runtime_error("DESCRIPTOR SET ERROR: Please add an image sampler before adding images");

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = m_context->getMaxSamplerAnisotropy();
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        if(vkCreateSampler(m_context->getDevice(), &samplerInfo, nullptr, &m_imageSampler)) {
            throw std::runtime_error("RENDERER ERROR: Could not create image sampler");
        }
    }
}

void DescriptorSet::addImages(VkDescriptorType descriptorType, std::vector<VkImageView> &imageViews) {
    m_descriptors.resize(m_numDescriptors + 1);
    auto &descriptor = m_descriptors[m_numDescriptors];

    uint32_t offset = 0;
    if(m_numDescriptors > 0) {
        offset = m_descriptors[m_numDescriptors - 1].firstBinding + m_descriptors[m_numDescriptors - 1].numBindings;
    }
    descriptor.firstBinding = offset;
    descriptor.numBindings = 1;

    descriptor.type = descriptorType;
    descriptor.numImages = imageViews.size();
    descriptor.imageViews.resize(descriptor.numImages);
    std::copy(imageViews.begin(), imageViews.end(), descriptor.imageViews.begin());

    //m_numImageBindings += descriptor.numImages;
    m_numImageBindings++;
    m_numImages += descriptor.numImages;
    m_numDescriptors++;

    if(descriptor.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && m_imageSampler == VK_NULL_HANDLE) {
        //throw std::runtime_error("DESCRIPTOR SET ERROR: Please add an image sampler before adding images");

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = m_context->getMaxSamplerAnisotropy();
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        if(vkCreateSampler(m_context->getDevice(), &samplerInfo, nullptr, &m_imageSampler)) {
            throw std::runtime_error("RENDERER ERROR: Could not create image sampler");
        }
    }
}

void DescriptorSet::init() {
    createLayoutAndPool();
    std::cout << "   DESCRIPTOR SET: Created layout and pool" << std::endl;
    createSets();
    std::cout << "   DESCRIPTOR SET: Created sets" << std::endl;
}

void DescriptorSet::createLayoutAndPool() {
    auto numBindings = m_numBufferBindings + m_numImageBindings;
    std::vector<VkDescriptorSetLayoutBinding> bindings(numBindings);
    std::vector<VkDescriptorPoolSize> poolSizes(numBindings);

    for(auto &descriptor : m_descriptors) {
        for(uint32_t b=0; b<descriptor.numBindings; b++) {
            bindings[descriptor.firstBinding + b].binding = descriptor.firstBinding + b;
            bindings[descriptor.firstBinding + b].descriptorType = descriptor.type;
            bindings[descriptor.firstBinding + b].descriptorCount = glm::max(descriptor.numImages, (uint32_t)1);
            bindings[descriptor.firstBinding + b].stageFlags = descriptor.type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_ALL;
            bindings[descriptor.firstBinding + b].pImmutableSamplers = nullptr;

            poolSizes[descriptor.firstBinding + b].type = descriptor.type;
            poolSizes[descriptor.firstBinding + b].descriptorCount = m_numFramesInFlight * glm::max(descriptor.numImages, (uint32_t)1);
        }
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = numBindings;
    layoutInfo.pBindings = bindings.data();

    if(vkCreateDescriptorSetLayout(m_context->getDevice(), &layoutInfo, nullptr, &m_layout) != VK_SUCCESS) {
        throw std::runtime_error("DESCRIPTOR SET ERROR: Could not create descriptor set layout.");
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = numBindings;
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = m_numFramesInFlight;

    if(vkCreateDescriptorPool(m_context->getDevice(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        throw std::runtime_error("DESCRIPTOR SET ERROR: Could not create descriptor pool.");
    }
}

void DescriptorSet::createSets() {
    auto numBindings = m_numBufferBindings + m_numImageBindings;
    std::vector<VkWriteDescriptorSet> writes(numBindings);
    std::vector<VkDescriptorBufferInfo> bufferInfos(m_numBufferBindings);
    std::vector<VkDescriptorImageInfo> imageInfos(m_numImages);

    uint32_t bufferIndex = 0;
    uint32_t imageIndex = 0;
    for(auto &descriptor : m_descriptors) {
        for(uint32_t b=0; b<descriptor.numBindings; b++) {
            writes[descriptor.firstBinding + b].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[descriptor.firstBinding + b].dstBinding = descriptor.firstBinding + b;
            writes[descriptor.firstBinding + b].dstArrayElement = 0;
            writes[descriptor.firstBinding + b].descriptorType = descriptor.type;
            writes[descriptor.firstBinding + b].descriptorCount = glm::max(descriptor.numImages, (uint32_t)1);

            if(descriptor.numImages == 0) {
                bufferInfos[bufferIndex].offset = static_cast<VkDeviceSize>(0);
                bufferInfos[bufferIndex].range = descriptor.bufferSize;
                writes[descriptor.firstBinding + b].pBufferInfo = &bufferInfos[bufferIndex];
                bufferIndex++;
            } else {
                for(uint32_t i=0; i<descriptor.numImages; i++) {
                    imageInfos[imageIndex + i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfos[imageIndex + i].imageView = descriptor.imageViews[i];
                    if(descriptor.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                        imageInfos[imageIndex + i].sampler = m_imageSampler;
                    } else {
                        imageInfos[imageIndex + i].sampler = nullptr;
                    }
                }
                writes[descriptor.firstBinding + b].pImageInfo = &imageInfos[imageIndex];
                imageIndex += descriptor.numImages;
            }
        }
    }

    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.descriptorPool = m_pool;
    setInfo.descriptorSetCount = 1;
    setInfo.pSetLayouts = &m_layout;

    m_sets.resize(m_numFramesInFlight);
    for(uint32_t frame=0; frame<m_numFramesInFlight; frame++) {
        //create set
        if(vkAllocateDescriptorSets(m_context->getDevice(), &setInfo, &m_sets[frame]) != VK_SUCCESS) {
            throw std::runtime_error("DESCRIPTOR SET ERROR: Could not allocate descriptor sets.");
        }

        //populate set
        bufferIndex = 0;
        for(auto &descriptor : m_descriptors) {
            for(uint32_t b=0; b<descriptor.numBindings; b++) {
                writes[descriptor.firstBinding + b].dstSet = m_sets[frame];
                if(descriptor.numImages == 0) {
                    bufferInfos[bufferIndex].buffer = descriptor.buffers[(frame + m_numFramesInFlight - (descriptor.numBindings-1-b)) % m_numFramesInFlight];
                    bufferIndex++;
                }
            }
        }

        vkUpdateDescriptorSets(m_context->getDevice(), writes.size(), writes.data(), 0, nullptr);
    }

}

void DescriptorSet::updateBuffer(std::string name, uint32_t frameIndex, void* data) {
    uint32_t descriptorIndex = 0;
    while(descriptorIndex < m_numDescriptors) {
        if(m_descriptors[descriptorIndex].name == name) {
            memcpy(m_descriptors[descriptorIndex].buffersMapped[frameIndex], data, m_descriptors[descriptorIndex].bufferSize);
            return;
        }
        descriptorIndex++;
    }
    throw std::runtime_error("DESCRIPTOR SET ERROR: Could not find a buffer named " + name);
}

void DescriptorSet::clearBuffer(std::string name, VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    uint32_t descriptorIndex = 0;
    while(descriptorIndex < m_numDescriptors) {
        if(m_descriptors[descriptorIndex].name == name) {
            vkCmdFillBuffer(commandBuffer, m_descriptors[descriptorIndex].buffers[frameIndex], 0, m_descriptors[descriptorIndex].bufferSize, 0);
            return;
        }
        descriptorIndex++;
    }
    throw std::runtime_error("DESCRIPTOR SET ERROR: Could not find a buffer named " + name);
}

void DescriptorSet::copyBufferFromLastFrame(std::string name, uint32_t frameIndex) {
    uint32_t descriptorIndex = 0;
    while(descriptorIndex < m_numDescriptors) {
        if(m_descriptors[descriptorIndex].name == name) {
            auto lastFrame = (frameIndex + (m_numFramesInFlight - 1)) % m_numFramesInFlight;
            m_context->copyBuffer(m_descriptors[descriptorIndex].buffers[lastFrame], m_descriptors[descriptorIndex].buffers[frameIndex], m_descriptors[descriptorIndex].bufferSize);
            return;
        }
        descriptorIndex++;
    }
    throw std::runtime_error("DESCRIPTOR SET ERROR: Could not find a buffer named " + name);
}

void DescriptorSet::cleanUp() {
    vkDestroyDescriptorPool(m_context->getDevice(), m_pool, nullptr);
    vkDestroyDescriptorSetLayout(m_context->getDevice(), m_layout, nullptr);

    for(auto &descriptor : m_descriptors) {
        for(auto buffer : descriptor.buffers) {
            vkDestroyBuffer(m_context->getDevice(), buffer, nullptr);
        }
        for(auto bufferMemory : descriptor.memory) {
            vkFreeMemory(m_context->getDevice(), bufferMemory, nullptr);
        }
    }

    if(m_imageSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_context->getDevice(), m_imageSampler, nullptr);
    }
}