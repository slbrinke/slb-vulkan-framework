#include "RenderStep.h"

RenderStep::RenderStep(std::shared_ptr<Context> &context, uint32_t numFramesInFlight)
: m_context(context), m_numFramesInFlight(numFramesInFlight) {

}

RenderStep::~RenderStep() {
    m_context = nullptr;
}

VkPipelineLayout RenderStep::getPipelineLayout() {
    return m_pipelineLayout;
}

RenderMode RenderStep::getRenderMode() {
    return m_renderMode;
}

uint32_t RenderStep::getRenderSize() {
    return m_renderSize;
}

uint32_t RenderStep::getOutputIndex() {
    return m_outputIndex;
}

uint32_t RenderStep::getSubPassIndex() {
    return m_subPassIndex;
}

void RenderStep::setName(std::string name) {
    m_name = name;
}

void RenderStep::setRenderMode(RenderMode mode, uint32_t renderSize) {
    m_renderMode = mode;
    m_renderSize = renderSize;
}

void RenderStep::createShaderModules(const std::vector<std::string> &shaderFiles, std::vector<DescriptorSet> &descriptorSets, std::vector<uint32_t> &sceneCounts) {
    for(size_t shader=0; shader<shaderFiles.size(); shader++) {
        ResourceLoader::findRequiredDescriptorSets(shaderFiles[shader], m_requiredDescriptorSets);
    }

    m_shaderModules.resize(shaderFiles.size());
    for(size_t shader=0; shader<shaderFiles.size(); shader++) {
        auto compiledName = ResourceLoader::compileShader(shaderFiles[shader], m_requiredDescriptorSets, sceneCounts);
        auto code = ResourceLoader::loadFile(compiledName);
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        if(vkCreateShaderModule(m_context->getDevice(), &createInfo, nullptr, &m_shaderModules[shader]) != VK_SUCCESS) {
            throw std::runtime_error("RENDER STEP ERROR: Could not create shader module: " + shaderFiles[shader]);
        }
        m_shaderStages.emplace_back(getShaderStage(shaderFiles[shader]));
    }

    m_descriptorSets.resize(m_numFramesInFlight);
    for(auto descriptorSetIndex : m_requiredDescriptorSets) {
        if(descriptorSetIndex < descriptorSets.size()) {
            m_descriptorSetLayouts.emplace_back(descriptorSets[descriptorSetIndex].getLayout());
            for(uint32_t frame=0; frame<m_numFramesInFlight; frame++) {
                m_descriptorSets[frame].emplace_back(descriptorSets[descriptorSetIndex].getSet(frame));
            }
        }
    }
}

VkShaderStageFlagBits RenderStep::getShaderStage(const std::string &fileName) {
    auto periodPos = fileName.find_last_of('.');
    auto fileType = fileName.substr(periodPos + 1, fileName.length());
    
    if(fileType == "vert") {
        return VK_SHADER_STAGE_VERTEX_BIT;
    }
    if(fileType == "geom") {
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    }
    if(fileType == "frag") {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if(fileType == "comp") {
        return VK_SHADER_STAGE_COMPUTE_BIT;
    }

    throw std::runtime_error("RENDER STEP ERROR: Unknown shader file type " + fileType);
}

void RenderStep::setCullMode(VkCullModeFlags mode) {
    m_cullMode = mode;
}

void RenderStep::enableBlending() {
    m_useBlending = true;
}

void RenderStep::initRenderStep(RenderOutput &output, uint32_t subPassIndex) {
    m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    m_outputIndex = output.getIndex();
    m_subPassIndex = subPassIndex;

    //shaders
    std::vector<VkPipelineShaderStageCreateInfo> shaderInfos(m_shaderModules.size());
    for(size_t shader=0; shader<m_shaderModules.size(); shader++) {
        shaderInfos[shader].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderInfos[shader].stage = m_shaderStages[shader];
        shaderInfos[shader].module = m_shaderModules[shader];
        shaderInfos[shader].pName = "main";
    }
    pipelineInfo.stageCount = shaderInfos.size();
    pipelineInfo.pStages = shaderInfos.data();

    //vertex input as defined in Mesh
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    //input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = m_primitiveTopology;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;

    //dynamic states: viewport and scissor
    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    pipelineInfo.pDynamicState = &dynamicState;
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    pipelineInfo.pViewportState = &viewportState;

    //rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = m_cullMode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE; //optional for shadow mapping
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
    pipelineInfo.pRasterizationState = &rasterizer;

    //multisampling for antialiasing
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    if(output.subPassUsesMultisampling(subPassIndex)) {
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = m_context->getMaxSamples();
        multisampling.minSampleShading = 1.0f;
    } else {
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
    }
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
    pipelineInfo.pMultisampleState = &multisampling;

    //depth test
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    if(output.subPassUsesDepth(subPassIndex)) {
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = m_useDepth ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = m_useDepth ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};
        pipelineInfo.pDepthStencilState = &depthStencil;
    } else {
        pipelineInfo.pDepthStencilState = nullptr;
    }

    //color blending
    auto numColorAttachments = output.getNumSubPassColorAttachments(subPassIndex);
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(numColorAttachments);
    for(uint32_t i=0; i<numColorAttachments; i++) {
        colorBlendAttachments[i].colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT
                | VK_COLOR_COMPONENT_G_BIT
                | VK_COLOR_COMPONENT_B_BIT
                | VK_COLOR_COMPONENT_A_BIT;
        if(m_useBlending) {
            colorBlendAttachments[i].blendEnable = VK_TRUE;
            colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
        } else {
            colorBlendAttachments[i].blendEnable = VK_FALSE;
            colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
        }
    }
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = colorBlendAttachments.size();
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    pipelineInfo.pColorBlendState = &colorBlending;

    //pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    //descriptor set layouts
    if(output.subPassHasInputs(m_subPassIndex)) {
        m_descriptorSetLayouts.emplace_back(output.getInputDescriptorSet(m_subPassIndex).getLayout());
        for(uint32_t frame=0; frame<m_numFramesInFlight; frame++) {
            m_descriptorSets[frame].emplace_back(output.getInputDescriptorSet(m_subPassIndex).getSet(frame));
        }
    }
    pipelineLayoutInfo.setLayoutCount = m_descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayouts.data();

    //push constants
    std::vector<VkPushConstantRange> pushConstants(1);
    pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstants[0].offset = 0;
    pushConstants[0].size = sizeof(SceneNodeConstants);
    pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

    if(vkCreatePipelineLayout(m_context->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("RENDER STEP ERROR: Could not create pipeline layout");
    }
    pipelineInfo.layout = m_pipelineLayout;

    pipelineInfo.renderPass = output.getRenderPass();
    pipelineInfo.subpass = subPassIndex;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(m_context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("RENDER STEP ERROR: Could not create graphics pipeline");
    }
}

void RenderStep::start(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    VkDebugUtilsLabelEXT markerInfo{};
    markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    markerInfo.pLabelName = m_name.c_str();
    auto beginFunc = (PFN_vkCmdBeginDebugUtilsLabelEXT)m_context->getExtensionFunction("vkCmdBeginDebugUtilsLabelEXT");
    if(beginFunc != nullptr) {
        beginFunc(commandBuffer, &markerInfo);
    }

    vkCmdBindPipeline(commandBuffer, m_bindPoint, m_pipeline);
    vkCmdBindDescriptorSets(commandBuffer, m_bindPoint, m_pipelineLayout, 0, m_descriptorSets[frameIndex].size(), m_descriptorSets[frameIndex].data(), 0, nullptr);
}

void RenderStep::end(VkCommandBuffer commandBuffer) {
    auto endFunc = (PFN_vkCmdEndDebugUtilsLabelEXT)m_context->getExtensionFunction("vkCmdEndDebugUtilsLabelEXT");
    if(endFunc != nullptr) {
        endFunc(commandBuffer);
    }
}

void RenderStep::cleanUp() {
    vkDestroyPipeline(m_context->getDevice(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_context->getDevice(), m_pipelineLayout, nullptr);
    for(auto shaderModule : m_shaderModules) {
        vkDestroyShaderModule(m_context->getDevice(), shaderModule, nullptr);
    }
}