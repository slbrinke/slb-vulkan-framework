#include "StandardRenderers.h"

ForwardRenderer::ForwardRenderer(std::shared_ptr<Context> &context, std::shared_ptr<Camera> &camera, std::shared_ptr<Scene> &scene)
: Renderer(context, camera, scene) {
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

    for(auto &output : m_renderOutput) {
        output.init();
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