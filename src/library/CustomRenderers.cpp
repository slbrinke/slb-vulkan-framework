#include "CustomRenderers.h"

PlantRenderer::PlantRenderer(std::shared_ptr<Context> &context, std::shared_ptr<Scene> &scene)
: Renderer(context, scene) {
    setUpRenderOutput();
    setUpDescriptorSets();
    setUpRenderSteps();
    createCommandBuffers();
    createSyncObjects();
}

void PlantRenderer::setUpRenderOutput() {
    m_renderOutput.emplace_back(m_context, m_numSwapChainImages, m_imageExtent, 1, true);
    auto bgColor = m_scene->getBackgroundColor();
    m_renderOutput.back().addSwapChainAttachment(m_swapChain, m_swapChainFormat, glm::vec4(bgColor, 1.0f));
    m_renderOutput.back().addDepthAttachment(m_depthFormat, 1.0f, false);

    for(uint32_t o=0; o<m_renderOutput.size(); o++) {
        m_renderOutput[o].init(o);
    }
}

void PlantRenderer::setUpRenderSteps() {
    auto sceneCounts = m_scene->getSceneCounts();

    m_computeSteps.emplace_back(m_context, m_numSwapChainImages);
    m_computeSteps.back().setName("Update Plant Modules");
    m_computeSteps.back().createShaderModules(
        {"plants/updatePlantModules.comp"},
        m_descriptorSets, sceneCounts);
    m_computeSteps.back().setComputeMode(computeSimple, m_scene->getMaxPlantModules());
    m_computeSteps.back().initComputeStep();

    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Render Geometry to Screen");
    
    m_renderSteps.back().createShaderModules(
        {"forward/forwardPBShading.vert", "forward/forwardPBShading.frag"},
        m_descriptorSets, sceneCounts);
    m_renderSteps.back().initRenderStep(m_renderOutput[0], 0);

    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Visualize Branch Structure");
    m_renderSteps.back().createShaderModules(
        {"plants/visualizeBranch.vert", "plants/visualizeBranch.geom", "plants/visualizeBranch.frag"},
        m_descriptorSets, sceneCounts);
    m_renderSteps.back().setRenderMode(renderInstancedPoint, m_scene->getNumBranches());
    m_renderSteps.back().setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    m_renderSteps.back().initRenderStep(m_renderOutput[0], 0);

    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Visualize Modules");
    m_renderSteps.back().createShaderModules(
        {"plants/visualizeModule.vert", "plants/visualizeModule.geom", "plants/visualizeModule.frag"},
        m_descriptorSets, sceneCounts);
    m_renderSteps.back().setRenderMode(renderInstancedPoint, m_scene->getMaxPlantModules());
    m_renderSteps.back().setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    m_renderSteps.back().initRenderStep(m_renderOutput[0], 0);
}