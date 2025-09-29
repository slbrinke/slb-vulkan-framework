#include "Renderer.h"

Renderer::Renderer(std::shared_ptr<Context> &context, std::shared_ptr<Camera> &camera, std::shared_ptr<Scene> &scene)
: m_context(context), m_camera(camera), m_scene(scene) {
    createSwapChain();

    m_depthFormat = m_context->findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    m_scene->init(m_context);
}

Renderer::~Renderer() {
    m_context = nullptr;
}

void Renderer::createSwapChain() {
    SwapChainSupport swapChainSupport = m_context->getSwapChainSupport();

    //choose surface format
    VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
    for(const auto& availableFormat : swapChainSupport.formats) {
        if(availableFormat.format == m_swapChainFormat
           && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }
    m_swapChainFormat = surfaceFormat.format;

    //choose present mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(const auto& availablePresentMode : swapChainSupport.presentModes) {
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
        }
    }

    m_imageExtent = swapChainSupport.capabilities.currentExtent;
    if(m_imageExtent.width == std::numeric_limits<uint32_t>::max()) {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_context->getWindow().get(), &width, &height);
        m_imageExtent.width = static_cast<uint32_t>(width);
        m_imageExtent.height = static_cast<uint32_t>(height);
        m_imageExtent.width = glm::clamp(m_imageExtent.width, swapChainSupport.capabilities.minImageExtent.width, swapChainSupport.capabilities.maxImageExtent.width);
        m_imageExtent.width = glm::clamp(m_imageExtent.height, swapChainSupport.capabilities.minImageExtent.height, swapChainSupport.capabilities.maxImageExtent.height);
    }
    m_viewport.width = static_cast<float>(m_imageExtent.width);
    m_viewport.height = static_cast<float>(m_imageExtent.height);
    m_scissor.extent = m_imageExtent;

    m_numSwapChainImages = swapChainSupport.capabilities.minImageCount + 1; //number of images being swapped
    if(swapChainSupport.capabilities.maxImageCount > 0 && m_numSwapChainImages > swapChainSupport.capabilities.maxImageCount) {
        m_numSwapChainImages = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainInfo{};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = m_context->getSurface();
    swapChainInfo.minImageCount = m_numSwapChainImages;
    swapChainInfo.imageFormat = surfaceFormat.format;
    swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainInfo.imageExtent = m_imageExtent;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    std::array<uint32_t,2> queueFamilyIndices = m_context->getQueueFamilyIndices();
    if(queueFamilyIndices[0] != queueFamilyIndices[1]) {
        swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainInfo.queueFamilyIndexCount = 2;
        swapChainInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }
    else {
        swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainInfo.queueFamilyIndexCount = 0;
        swapChainInfo.pQueueFamilyIndices = nullptr;
    }

    swapChainInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.clipped = VK_TRUE;
    swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(m_context->getDevice(), &swapChainInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
        throw std::runtime_error("SWAP CHAIN ERROR: Could not create swapchain.");
    }

    //retrieve number of frames in flight from swap chain
    vkGetSwapchainImagesKHR(m_context->getDevice(), m_swapChain, &m_numSwapChainImages, nullptr);

    std::cout << "   RENDERER: Created swapchain with " << m_numSwapChainImages << " images" << std::endl;
}

void Renderer::setUpRenderOutput() {
    //implemented in subclasses
}

void Renderer::setUpDescriptorSets() {
    m_descriptorSets.emplace_back(m_context, m_numSwapChainImages);
    m_descriptorSets.back().addBuffer("Camera", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(CameraUniforms), false);

    for(auto &descriptorSet : m_descriptorSets) {
        descriptorSet.init();
    }
}

void Renderer::setUpRenderSteps() {
    //implemented in subclasses
}

void Renderer::createCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_context->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = m_numSwapChainImages;

    m_graphicsCommandBuffers.resize(m_numSwapChainImages);
    if(vkAllocateCommandBuffers(m_context->getDevice(), &allocInfo, m_graphicsCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("RENDERER ERROR: Could not create graphics command buffers");
    }

    m_computeCommandBuffers.resize(m_numSwapChainImages);
    if(vkAllocateCommandBuffers(m_context->getDevice(), &allocInfo, m_computeCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("RENDERER ERROR: Could not create compute command buffers");
    }

    std::cout << "   RENDERER: Created command buffers" << std::endl;

}

void Renderer::createSyncObjects() {
    m_imageAvailableSemaphores.resize(m_numSwapChainImages);
    m_computeFinishedSemaphores.resize(m_numSwapChainImages);
    m_graphicsFinishedSemaphores.resize(m_numSwapChainImages);
    m_computeInFlightFences.resize(m_numSwapChainImages);
    m_graphicsInFlightFences.resize(m_numSwapChainImages);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //in order for the first frame to work

    for (size_t i = 0; i < m_numSwapChainImages; i++) {
        if (vkCreateSemaphore(m_context->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_context->getDevice(), &semaphoreInfo, nullptr, &m_computeFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_context->getDevice(), &semaphoreInfo, nullptr, &m_graphicsFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_context->getDevice(), &fenceInfo, nullptr, &m_computeInFlightFences[i]) != VK_SUCCESS ||
            vkCreateFence(m_context->getDevice(), &fenceInfo, nullptr, &m_graphicsInFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("RENDERER ERROR: Could not create synchronization objects.");
        }
    }

    std::cout << "   RENDERER: Created synchronization objects" << std::endl;
}

void Renderer::update() {
    uint32_t frameIndex = m_currentFrame % m_numSwapChainImages;

    //update uniforms
    CameraUniforms camUniforms{
        m_camera->getViewMatrix(),
        m_camera->getProjectionMatrix()
    };
    m_descriptorSets[0].updateBuffer("Camera", frameIndex, &camUniforms);

    compute();
}

void Renderer::compute() {
    if(false) { //for now
        uint32_t frameIndex = m_currentFrame % m_numSwapChainImages;
        vkWaitForFences(m_context->getDevice(), 1, &m_computeInFlightFences[frameIndex], VK_TRUE, UINT64_MAX);

        vkResetFences(m_context->getDevice(), 1, &m_computeInFlightFences[frameIndex]);

        vkResetCommandBuffer(m_computeCommandBuffers[frameIndex], 0);

        //begin compute command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if(vkBeginCommandBuffer(m_computeCommandBuffers[frameIndex], &beginInfo) != VK_SUCCESS) {
            std::cout << "RENDERER ERROR: Could not record compute command buffer" << std::endl;
        }

        //vkCmdResetQueryPool(m_computeCommandBuffers[frameIndex], m_computeTimeQueryPool, 2 * (frameIndex), 2);
        //vkCmdWriteTimestamp(m_computeCommandBuffers[frameIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_computeTimeQueryPool, 2 * (frameIndex));
        
        recordComputeCommandBuffer();

        //vkCmdWriteTimestamp(m_computeCommandBuffers[frameIndex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_computeTimeQueryPool, 2 * frameIndex + 1);

        //end command buffer
        if(vkEndCommandBuffer(m_computeCommandBuffers[frameIndex]) != VK_SUCCESS) {
            std::cout << "RENDERER ERROR: Could not record compute command buffer" << std::endl;
        }

        //submit compute command buffer to queue
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_computeCommandBuffers[frameIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_computeFinishedSemaphores[frameIndex];

        if(vkQueueSubmit(m_context->getComputeQueue(), 1, &submitInfo, m_computeInFlightFences[frameIndex]) != VK_SUCCESS) {
            throw std::runtime_error("RENDERER ERROR: Could not submit compute command buffer");
        }
    }
}

void Renderer::recordComputeCommandBuffer() {
    //TO DO
}

void Renderer::render() {
    uint32_t frameIndex = m_currentFrame % m_numSwapChainImages;
    vkWaitForFences(m_context->getDevice(), 1, &m_graphicsInFlightFences[frameIndex], VK_TRUE, UINT64_MAX);

    //fetch next image from swapchain
    VkResult result = vkAcquireNextImageKHR(m_context->getDevice(), m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &m_swapChainImageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        //resizeScreen();
        return;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("RENDERER ERROR: Could not acquire swap chain image.");
    }

    vkResetFences(m_context->getDevice(), 1, &m_graphicsInFlightFences[frameIndex]);

    vkResetCommandBuffer(m_graphicsCommandBuffers[frameIndex], 0);

    //begin graphics command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if(vkBeginCommandBuffer(m_graphicsCommandBuffers[frameIndex], &beginInfo) != VK_SUCCESS) {
        std::cout << "RENDERER ERROR: Could not record draw command buffer" << std::endl;
    }

    //vkCmdResetQueryPool(m_graphicsCommandBuffers[frameIndex], m_graphicsTimeQueryPool, 2 * (frameIndex), 2);
    //vkCmdWriteTimestamp(m_graphicsCommandBuffers[frameIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_graphicsTimeQueryPool, 2 * frameIndex);

    recordGraphicsCommandBuffer();

    //vkCmdWriteTimestamp(m_graphicsCommandBuffers[frameIndex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_graphicsTimeQueryPool, 2 * frameIndex + 1);

    if(vkEndCommandBuffer(m_graphicsCommandBuffers[frameIndex]) != VK_SUCCESS) {
        std::cout << "RENDERER ERROR: Could not record command buffer" << std::endl;
    }

    std::vector<VkSemaphore> waitSemaphores = {m_imageAvailableSemaphores[frameIndex]};
    if(false) { //!m_computePipelines.empty()
        waitSemaphores.emplace_back(m_computeFinishedSemaphores[frameIndex]);
    }
    //VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame], m_computeFinishedSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    //VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data(); //wait for swap chain image and compute shaders
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_graphicsCommandBuffers[frameIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_graphicsFinishedSemaphores[frameIndex]; //signal render done after

    if(vkQueueSubmit(m_context->getGraphicsQueue(), 1, &submitInfo, m_graphicsInFlightFences[frameIndex]) != VK_SUCCESS) {
        throw std::runtime_error("RENDERER ERROR: Could not submit draw command buffer");
    }

    //submit result back to swap chain
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_graphicsFinishedSemaphores[frameIndex];
    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_swapChainImageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_context->getPresentQueue(), &presentInfo);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_frameBufferResized) {
        m_frameBufferResized = false;
        //resizeScreen();
    }
    else if(result != VK_SUCCESS) {
        throw std::runtime_error("RENDERER ERROR: Could not present swap chain image.");
    }

    m_currentFrame++;
}

void Renderer::recordGraphicsCommandBuffer() {
    uint32_t frameIndex = m_currentFrame % m_numSwapChainImages;
    auto commandBuffer = m_graphicsCommandBuffers[frameIndex];

    //uint32_t renderPassIndex = 0;
    //uint32_t subPassIndex = 0;
    //m_renderOutput[renderPassIndex].start(commandBuffer, frameIndex);

    m_renderOutput[0].start(commandBuffer, frameIndex);
    m_renderSteps[0].start(commandBuffer, frameIndex);
    m_scene->renderMeshes(commandBuffer);
    m_renderSteps[0].end(commandBuffer);
    m_renderOutput[1].end(commandBuffer);
}

void Renderer::cleanUp() {
    for(uint32_t i=0; i<m_numSwapChainImages; i++) {
        vkDestroySemaphore(m_context->getDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_context->getDevice(), m_computeFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_context->getDevice(), m_graphicsFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_context->getDevice(), m_computeInFlightFences[i], nullptr);
        vkDestroyFence(m_context->getDevice(), m_graphicsInFlightFences[i], nullptr);
    }
    for(auto &descriptorSet : m_descriptorSets) {
        descriptorSet.cleanUp();
    }
    for(auto &step : m_renderSteps) {
        step.cleanUp();
    }
    for(auto &output : m_renderOutput) {
        output.cleanUp();
    }
    if(m_swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_context->getDevice(), m_swapChain, nullptr);
    }
}

SimpleRenderer::SimpleRenderer(std::shared_ptr<Context> &context, std::shared_ptr<Camera> &camera, std::shared_ptr<Scene> &scene)
: Renderer(context, camera, scene) {
    setUpRenderOutput();
    setUpDescriptorSets();
    setUpRenderSteps();
    createCommandBuffers();
    createSyncObjects();
}

void SimpleRenderer::setUpRenderOutput() {
    m_renderOutput.emplace_back(m_context, m_numSwapChainImages, m_imageExtent, 1, false);
    m_renderOutput.back().addSwapChainAttachment(m_swapChain, m_swapChainFormat, glm::vec4(1.0, 0.3, 0.0, 1.0));

    for(auto &output : m_renderOutput) {
        output.init();
    }
}

void SimpleRenderer::setUpRenderSteps() {
    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Simple Rendering");
    m_renderSteps.back().createShaderModules({"simple.vert", "simple.frag"}, m_descriptorSets);
    m_renderSteps.back().initRenderStep(m_renderOutput[0], 0);
}