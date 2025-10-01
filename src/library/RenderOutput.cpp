#include "RenderOutput.h"

RenderOutput::RenderOutput(std::shared_ptr<Context> &context, uint32_t numFramesInFlight, VkExtent2D imageExtent, uint32_t numLayers, bool useMultisampling)
: m_context(context), m_numFramesInFlight(numFramesInFlight), m_imageExtent(imageExtent), m_numLayers(numLayers) {
    //there has to be at least one subpass
    addSubPass(useMultisampling);

    m_viewport.width = static_cast<float>(m_imageExtent.width);
    m_viewport.height = static_cast<float>(m_imageExtent.height);
    m_scissor.extent = m_imageExtent;
}

RenderOutput::~RenderOutput() {
    m_context = nullptr;
}

uint32_t RenderOutput::getNumSubPassColorAttachments(uint32_t subPassIndex) {
    if(subPassIndex < m_numSubPasses) {
        return m_subPasses[subPassIndex].numColorAttachments;
    }

    throw std::runtime_error("RENDER OUTPUT ERROR: There is no subpass with index " + subPassIndex);
}

bool RenderOutput::subPassUsesDepth(uint32_t subPassIndex) {
    if(subPassIndex < m_numSubPasses) {
        return m_subPasses[subPassIndex].useDepth;
    }

    throw std::runtime_error("RENDER OUTPUT ERROR: There is no subpass with index " + subPassIndex);
}

bool RenderOutput::subPassUsesMultisampling(uint32_t subPassIndex) {
    if(subPassIndex < m_numSubPasses) {
        return m_subPasses[subPassIndex].useMultisampling;
    }

    throw std::runtime_error("RENDER OUTPUT ERROR: There is no subpass with index " + subPassIndex);
}

VkRenderPass RenderOutput::getRenderPass() {
    return m_renderPass;
}

void RenderOutput::addSubPass(bool useMultisampling) {
    m_subPasses.resize(m_numSubPasses + 1);
    auto &subPass = m_subPasses[m_numSubPasses];
    m_numSubPasses++;

    subPass.firstAttachment = m_numAttachments;
    subPass.useMultisampling = useMultisampling;
}

void RenderOutput::addColorAttachment(VkFormat colorFormat, glm::vec4 clearColor, bool isExternalInput) {
    m_attachments.resize(m_numAttachments + 1);
    auto &attachment = m_attachments[m_numAttachments];
    auto &subPass = m_subPasses[m_numSubPasses - 1];

    //properties of the new attachment image
    attachment.mainIndex = m_images.size();
    m_images.emplace_back(m_imageExtent.width, m_imageExtent.height, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    m_images[attachment.mainIndex].setNumLayers(m_numLayers);
    m_images[attachment.mainIndex].setFormat(colorFormat);
    m_images[attachment.mainIndex].setAspect(VK_IMAGE_ASPECT_COLOR_BIT);
    m_images[attachment.mainIndex].setLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_images[attachment.mainIndex].setFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    if(subPass.useMultisampling) {
        attachment.useMultisampling = true;
        m_images[attachment.mainIndex].enableMultisampling();
        subPass.useMultisampling = true;

        //if multisampling is used add an image to resolve to
        if(isExternalInput) {
            attachment.hasResolve = true;
            attachment.resolveIndex = m_images.size();
            m_images.emplace_back(m_imageExtent.width, m_imageExtent.height, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            m_images[attachment.resolveIndex].setNumLayers(m_numLayers);
            m_images[attachment.resolveIndex].setFormat(colorFormat);
            m_images[attachment.resolveIndex].setAspect(VK_IMAGE_ASPECT_COLOR_BIT);
            m_images[attachment.resolveIndex].setLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            m_images[attachment.resolveIndex].setFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            m_numResolveImages++;
        }
        m_numMultisampledImages++;
    }

    //certain settings have to be changed if the image is used as input outside of the renderpass
    if(isExternalInput) {
        if(subPass.useMultisampling) {
            m_images[attachment.resolveIndex].addUsage(VK_IMAGE_USAGE_SAMPLED_BIT);
            
            m_images[attachment.resolveIndex].setInitialLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            m_images[attachment.resolveIndex].setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        } else {
            m_images[attachment.mainIndex].addUsage(VK_IMAGE_USAGE_SAMPLED_BIT);

            m_images[attachment.mainIndex].setInitialLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            m_images[attachment.mainIndex].setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    //add clear attachment
    subPass.clearAttachments.resize(subPass.clearAttachments.size() + 1);
    subPass.clearAttachments.back().aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subPass.clearAttachments.back().colorAttachment = subPass.numColorAttachments;
    subPass.numColorAttachments++;
    subPass.clearAttachments.back().clearValue.color = {clearColor.x, clearColor.y, clearColor.z, clearColor.w};

    m_numAttachments++;
    subPass.numAttachments++;
}

void RenderOutput::addDepthAttachment(VkFormat depthFormat, float clearDepth, bool isExternalInput) {
    m_attachments.resize(m_numAttachments + 1);
    auto &attachment = m_attachments[m_numAttachments];
    auto &subPass = m_subPasses[m_numSubPasses - 1];

    //properties of the new attachment image
    attachment.mainIndex = m_images.size();
    m_images.emplace_back(m_imageExtent.width, m_imageExtent.height, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    m_images[attachment.mainIndex].setNumLayers(m_numLayers);
    m_images[attachment.mainIndex].setFormat(depthFormat);
    m_images[attachment.mainIndex].setAspect(VK_IMAGE_ASPECT_DEPTH_BIT);
    m_images[attachment.mainIndex].setLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    m_images[attachment.mainIndex].setFinalLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    if(subPass.useMultisampling) {
        attachment.useMultisampling = true;
        m_images[attachment.mainIndex].enableMultisampling();
        subPass.useMultisampling = true;

        if(isExternalInput) {
            throw std::runtime_error("RENDER OUTPUT ERROR: Depth attachment cannot be multisampled because depth resolve is not implemented here");
        }
        m_numMultisampledImages++;
    }

    if(isExternalInput) {
        m_images[attachment.mainIndex].addUsage(VK_IMAGE_USAGE_SAMPLED_BIT);

        m_images[attachment.mainIndex].setInitialLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_images[attachment.mainIndex].setFinalLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    //add clear attachment
    subPass.clearAttachments.resize(subPass.clearAttachments.size() + 1);
    subPass.clearAttachments.back().aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    subPass.clearAttachments.back().clearValue.depthStencil = {clearDepth, 0};

    m_numAttachments++;
    subPass.numAttachments++;
    subPass.useDepth = true;
}

void RenderOutput::addSwapChainAttachment(VkSwapchainKHR swapChain, VkFormat swapChainFormat, glm::vec4 clearColor) {
    m_attachments.resize(m_numAttachments + 1);
    auto &attachment = m_attachments[m_numAttachments];
    auto &subPass = m_subPasses[m_numSubPasses - 1];

    //properties of the new attachment image
    attachment.mainIndex = m_images.size();
    m_images.emplace_back(m_imageExtent.width, m_imageExtent.height, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    m_images[attachment.mainIndex].setFormat(swapChainFormat);
    m_images[attachment.mainIndex].setAspect(VK_IMAGE_ASPECT_COLOR_BIT);
    m_images[attachment.mainIndex].setLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    m_images[attachment.mainIndex].setFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    //if multisampling is used resolve to swap chain images
    if(subPass.useMultisampling) {
        attachment.useMultisampling = true;
        m_images[attachment.mainIndex].enableMultisampling();
        subPass.useMultisampling = true;

        attachment.hasResolve = true;
        attachment.resolveIndex = m_images.size();
        m_images.emplace_back(m_imageExtent.width, m_imageExtent.height, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        m_images[attachment.resolveIndex].setFormat(swapChainFormat);
        m_images[attachment.resolveIndex].setAspect(VK_IMAGE_ASPECT_COLOR_BIT);
        m_images[attachment.resolveIndex].useSwapChain(m_context, swapChain);
        m_images[attachment.resolveIndex].setLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        m_images[attachment.resolveIndex].setFinalLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        m_numResolveImages++;

        m_numMultisampledImages++;
    } else {
        m_images[attachment.mainIndex].useSwapChain(m_context, swapChain);

        m_images[attachment.mainIndex].setFinalLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    //add clear attachment
    subPass.clearAttachments.resize(subPass.clearAttachments.size() + 1);
    subPass.clearAttachments.back().aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subPass.clearAttachments.back().colorAttachment = subPass.numColorAttachments;
    subPass.numColorAttachments++;
    subPass.clearAttachments.back().clearValue.color = {clearColor.x, clearColor.y, clearColor.z, clearColor.w};

    m_numAttachments++;
    subPass.numAttachments++;
}

void RenderOutput::addSubPassInput(uint32_t srcSubPass, uint32_t srcAttachment) {
    //VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT has to be added in the image usage flags
    m_images[m_attachments[m_subPasses[srcSubPass].firstAttachment + srcAttachment].mainIndex].addUsage(VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
    
    //the indices are only stored for now
    //the required attachment references and subpass dependencies are handled in createSubPasses
    m_subPasses[m_numSubPasses - 1].subPassInputs.emplace_back(srcSubPass, srcAttachment);
    m_numSubPassInputs++;
}

void RenderOutput::addRenderPassInput(VkImageView imageView, bool isDepth) {
    //image view is only stored for now
    m_subPasses[m_numSubPasses - 1].externalInputs.emplace_back(imageView, isDepth);
}

void RenderOutput::init() {
    createAttachments();
    createSubPasses();
    createFramebuffers();
    std::cout << "   RENDER OUTPUT: Created output with " << m_numSubPasses << " subpasses and " << m_numAttachments << " attachments." << std::endl;
}

void RenderOutput::createAttachments() {
    for(auto &image : m_images) {
        //check if the images are already created and allocated (e.g. already retrieved from the swap chain)
        if(!image.hasHandle()) {
            image.createAndAllocate(m_context);
        }
        image.createViews(m_context);

        //transition to initial layout if necessary
        if(image.getInitialLayout() != VK_IMAGE_LAYOUT_UNDEFINED) {
            image.transitionLayout(m_context, VK_IMAGE_LAYOUT_UNDEFINED, image.getInitialLayout());
        }
    }
}

void RenderOutput::createSubPasses() {
    std::vector<VkSubpassDescription> spDescriptions(m_numSubPasses);
    std::vector<VkSubpassDependency> spDependencies(1);
    spDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    spDependencies[0].dstSubpass = 0;
    spDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    spDependencies[0].srcAccessMask = 0;
    spDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    spDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    spDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    std::vector<VkAttachmentDescription> aDescriptions(m_numAttachments + m_numResolveImages);
    std::vector<VkAttachmentReference> aReferences(m_numAttachments + m_numMultisampledImages);
    std::vector<VkAttachmentReference> iaReferences(m_numSubPassInputs);

    for(uint32_t i=0; i<m_images.size(); i++) {
        aDescriptions[i].format = m_images[i].getFormat();
        aDescriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
        aDescriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        aDescriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if(m_images[i].usesMultisampling()) {
            aDescriptions[i].samples = m_context->getMaxSamples();
            aDescriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        aDescriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        aDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        aDescriptions[i].initialLayout = m_images[i].getInitialLayout();
        aDescriptions[i].finalLayout = m_images[i].getFinalLayout();
    }

    uint32_t aOffset = 0;
    uint32_t iaOffset = 0;
    uint32_t depOffset = spDependencies.size();
    for(uint32_t sp=0; sp<m_numSubPasses; sp++) {
        auto &subPass = m_subPasses[sp];

        //subpass dependency for multisampling
        if(m_subPasses[sp].useMultisampling) {
            spDependencies.resize(depOffset + 1);
            spDependencies[depOffset].srcSubpass = VK_SUBPASS_EXTERNAL;
            spDependencies[depOffset].dstSubpass = sp;
            spDependencies[depOffset].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            spDependencies[depOffset].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            spDependencies[depOffset].srcAccessMask = 0;
            spDependencies[depOffset].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            spDependencies[depOffset].dependencyFlags = 0;
            depOffset++;
        }

        //subpass inputs
        uint32_t numInputs = subPass.subPassInputs.size();
        std::set<uint32_t> depSrcSubPasses;
        for(uint32_t i=0; i<numInputs; i++) {
            auto &input = subPass.subPassInputs[i];
            auto &source = m_attachments[m_subPasses[input.first].firstAttachment + input.second];
            iaReferences[iaOffset + i].attachment = source.mainIndex;
            iaReferences[iaOffset + i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if(depSrcSubPasses.find(input.first) == depSrcSubPasses.end()) {
                spDependencies.resize(depOffset + 1);
                spDependencies[depOffset].srcSubpass = input.first;
                spDependencies[depOffset].dstSubpass = sp;
                spDependencies[depOffset].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                spDependencies[depOffset].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                spDependencies[depOffset].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                if(input.second < m_subPasses[input.first].numColorAttachments) {
                    spDependencies[depOffset].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    spDependencies[depOffset].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                } else {
                    spDependencies[depOffset].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    spDependencies[depOffset].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                }
                depOffset++;
                depSrcSubPasses.emplace(input.first);
            }
            
        }
        spDescriptions[sp].inputAttachmentCount = numInputs;
        spDescriptions[sp].pInputAttachments = &iaReferences[iaOffset];
        iaOffset += numInputs;

        //external renderpass inputs
        for(uint32_t e=0; e<subPass.externalInputs.size(); e++) {
            spDependencies.resize(depOffset + 1);
            spDependencies[depOffset].srcSubpass = VK_SUBPASS_EXTERNAL;
            spDependencies[depOffset].dstSubpass = sp;
            spDependencies[depOffset].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            spDependencies[depOffset].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            spDependencies[depOffset].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            spDependencies[depOffset].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            depOffset++;
        }

        //attachment definitions
        uint32_t maOffset = 0;
        for(uint32_t a=0; a<subPass.numAttachments; a++) {
            auto &attachment = m_attachments[subPass.firstAttachment + a];

            aReferences[aOffset + a].attachment = attachment.mainIndex;
            aReferences[aOffset + a].layout = m_images[attachment.mainIndex].getLayout();
            if(attachment.useMultisampling) {
                if(attachment.hasResolve) {
                    aReferences[aOffset + subPass.numAttachments + maOffset].attachment = attachment.resolveIndex;
                    aReferences[aOffset + subPass.numAttachments + maOffset].layout = m_images[attachment.resolveIndex].getLayout(); //VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                } else {
                    aReferences[aOffset + subPass.numAttachments + maOffset].attachment = VK_ATTACHMENT_UNUSED;
                }
                maOffset++;
            }

        }
        spDescriptions[sp].colorAttachmentCount = subPass.numColorAttachments;
        spDescriptions[sp].pColorAttachments = &aReferences[aOffset];
        if(subPass.useDepth) {
            spDescriptions[sp].pDepthStencilAttachment = &aReferences[aOffset + subPass.numColorAttachments];
        }
        if(maOffset > 0) {
            spDescriptions[sp].pResolveAttachments = &aReferences[aOffset + subPass.numAttachments];
        }
        aOffset += subPass.numAttachments + maOffset;

    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(aDescriptions.size());
    renderPassInfo.pAttachments = aDescriptions.data();
    renderPassInfo.subpassCount = m_numSubPasses;
    renderPassInfo.pSubpasses = spDescriptions.data();
    renderPassInfo.dependencyCount = static_cast<uint32_t>(spDependencies.size());
    renderPassInfo.pDependencies = spDependencies.data();

    if(vkCreateRenderPass(m_context->getDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("RENDER OUTPUT ERROR: Could not create render pass.");
    }
}

void RenderOutput::createFramebuffers() {
    m_frameBuffers.resize(m_numFramesInFlight);
    for(uint32_t f=0; f<m_numFramesInFlight; f++) {
        std::vector<VkImageView> aViews(m_numAttachments + m_numResolveImages);
        for(uint32_t i=0; i<m_images.size(); i++) {
            aViews[i] = m_images[i].getView(f);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(aViews.size());
        framebufferInfo.pAttachments = aViews.data();
        framebufferInfo.width = m_imageExtent.width;
        framebufferInfo.height = m_imageExtent.height;
        framebufferInfo.layers = m_numLayers;

        if(vkCreateFramebuffer(m_context->getDevice(), &framebufferInfo, nullptr, &m_frameBuffers[f]) != VK_SUCCESS) {
            throw std::runtime_error("RENDER PASS ERROR: Could not create framebuffer " + std::to_string(f));
        }
    }
}

void RenderOutput::start(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.framebuffer = m_frameBuffers[frameIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = m_imageExtent;
    renderPassBeginInfo.renderPass = m_renderPass;
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(commandBuffer, 0, 1, &m_viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &m_scissor);

    switchSubPass(commandBuffer, 0);
}

void RenderOutput::switchSubPass(VkCommandBuffer commandBuffer, uint32_t dstSubPass) {
    if(dstSubPass < m_currentSubPass || dstSubPass >= m_numSubPasses) {
        throw std::runtime_error("RENDER OUTPUT ERROR: Subpasses need to be ordered according to how they are used");
    }

    while(m_currentSubPass < dstSubPass) {
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        m_currentSubPass++;
    }

    //clear
    VkClearRect entireFrame{
            {0, 0, m_imageExtent}, 0, m_numLayers,
    };
    vkCmdClearAttachments(commandBuffer, static_cast<uint32_t>(m_subPasses[m_currentSubPass].clearAttachments.size()), m_subPasses[m_currentSubPass].clearAttachments.data(), 1, &entireFrame);
}

void RenderOutput::end(VkCommandBuffer commandBuffer) {
    vkCmdEndRenderPass(commandBuffer);
    m_currentSubPass = 0;
}

void RenderOutput::cleanUp() {
    vkDestroyRenderPass(m_context->getDevice(), m_renderPass, nullptr);
    for(auto frameBuffer : m_frameBuffers) {
        vkDestroyFramebuffer(m_context->getDevice(), frameBuffer, nullptr);
    }
    for(auto &image : m_images) {
        image.cleanUp(m_context);
    }
}