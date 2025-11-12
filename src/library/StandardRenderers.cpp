#include "StandardRenderers.h"

ForwardRenderer::ForwardRenderer(std::shared_ptr<Context> &context, std::shared_ptr<Scene> &scene)
: Renderer(context, scene) {
    setUpRenderOutput();
    setUpDescriptorSets();
    setUpRenderSteps();
    createCommandBuffers();
    createSyncObjects();
}

void ForwardRenderer::setUpRenderOutput() {
    m_renderOutput.emplace_back(m_context, m_numSwapChainImages, m_imageExtent, 1, true);
    auto bgColor = m_scene->getBackgroundColor();
    m_renderOutput.back().addSwapChainAttachment(m_swapChain, m_swapChainFormat, glm::vec4(bgColor, 1.0f));
    m_renderOutput.back().addDepthAttachment(m_depthFormat, 1.0f, false);

    for(uint32_t o=0; o<m_renderOutput.size(); o++) {
        m_renderOutput[o].init(o);
    }
}

void ForwardRenderer::setUpRenderSteps() {
    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Render Geometry to Screen");
    auto sceneCounts = m_scene->getSceneCounts();
    m_renderSteps.back().createShaderModules(
        {"forward/forwardPBShading.vert", "forward/forwardPBShading.frag"},
        m_descriptorSets, sceneCounts);
    m_renderSteps.back().initRenderStep(m_renderOutput[0], 0);
}

DeferredRenderer::DeferredRenderer(std::shared_ptr<Context> &context, std::shared_ptr<Scene> &scene)
: Renderer(context, scene) {
    setUpDescriptorSets();
    setUpRenderOutput();
    setUpRenderSteps();
    createCommandBuffers();
    createSyncObjects();
}

void DeferredRenderer::setUpRenderOutput() {
    m_renderOutput.emplace_back(m_context, m_numSwapChainImages, VkExtent2D{m_shadowMapSize, m_shadowMapSize}, m_scene->getSceneCounts()[1], false);
    m_renderOutput.back().addDepthAttachment(m_depthFormat, 1.0f, true);
    m_renderOutput.back().init(m_renderOutput.size() - 1);

    m_renderOutput.emplace_back(m_context, m_numSwapChainImages, m_imageExtent, 1, true);
    //gbuffer
    m_renderOutput.back().addColorAttachment(VK_FORMAT_R16G16B16A16_UNORM, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), false);
    m_renderOutput.back().addColorAttachment(VK_FORMAT_R16G16B16A16_UNORM, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), false);
    m_renderOutput.back().addColorAttachment(VK_FORMAT_R16G16B16A16_UNORM, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), false);
    m_renderOutput.back().addDepthAttachment(m_depthFormat, 1.0f, false);
    //main shading
    m_renderOutput.back().addSubPass(true);
    m_renderOutput.back().addColorAttachment(VK_FORMAT_R16G16B16A16_UNORM, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), false);
    m_renderOutput.back().addRenderPassInput(m_renderOutput[0].getSubPassAttachment(0, 0), true);
    m_renderOutput.back().addSubPassInput(0, 0);
    m_renderOutput.back().addSubPassInput(0, 1);
    m_renderOutput.back().addSubPassInput(0, 2);
    m_renderOutput.back().addSubPassInput(0, 3);
    //final composition/post processing
    m_renderOutput.back().addSubPass(true);
    m_renderOutput.back().addSwapChainAttachment(m_swapChain, m_swapChainFormat, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    m_renderOutput.back().addSubPassInput(1, 0);
    m_renderOutput.back().init(m_renderOutput.size() - 1);
}

void DeferredRenderer::setUpRenderSteps() {
    auto sceneCounts = m_scene->getSceneCounts();

    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Render Geometry to Shadow Maps");
    m_renderSteps.back().createShaderModules(
        {"shadow/renderMeshToShadowMap.vert", "shadow/renderMeshToShadowMap.geom"},
        m_descriptorSets, sceneCounts);
    m_renderSteps.back().setRenderMode(renderMeshes, sceneCounts[1]);
    m_renderSteps.back().initRenderStep(m_renderOutput[0], 0);

    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Render Geometry to GBuffer");
    m_renderSteps.back().createShaderModules(
        {"deferred/deferredMeshToGBuffer.vert", "deferred/deferredMeshToGBuffer.frag"},
        m_descriptorSets, sceneCounts);
    m_renderSteps.back().initRenderStep(m_renderOutput[1], 0);

    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Render Light Proxy");
    m_renderSteps.back().createShaderModules(
        {"deferred/deferredLightProxy.vert", "deferred/deferredLightProxy.frag"},
        m_descriptorSets, sceneCounts);
    m_renderSteps.back().setRenderMode(renderLightProxies);
    m_renderSteps.back().setCullMode(VK_CULL_MODE_FRONT_BIT);
    m_renderSteps.back().enableBlending();
    m_renderSteps.back().initRenderStep(m_renderOutput[1], 1);

    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Final Composition");
    m_renderSteps.back().createShaderModules(
        {"deferred/finalComposition.vert", "deferred/finalComposition.frag"},
        m_descriptorSets, sceneCounts);
    m_renderSteps.back().setRenderMode(renderEnvMap);
    m_renderSteps.back().initRenderStep(m_renderOutput[1], 2);
}