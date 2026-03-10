#include "CustomRenderers.h"

PlantRenderer::PlantRenderer(std::shared_ptr<Context> &context, std::shared_ptr<Scene> &scene)
: Renderer(context, scene) {
    m_useVoxels = true;

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
    m_computeSteps.back().setName("Module Voxel Count");
    m_computeSteps.back().createShaderModules(
        {"plants/moduleVoxelCount.comp"},
        m_descriptorSets, sceneCounts);
    m_computeSteps.back().setComputeMode(computeSimple, m_scene->getMaxPlantModules());
    m_computeSteps.back().initComputeStep();

    m_computeSteps.emplace_back(m_context, m_numSwapChainImages);
    m_computeSteps.back().setName("Prefix Sum");
    m_computeSteps.back().createShaderModules(
        {"plants/prefixSum.comp"},
        m_descriptorSets, sceneCounts);
    m_computeSteps.back().setComputeMode(computeCascaded, m_numVoxels.x * m_numVoxels.y * m_numVoxels.z);
    m_computeSteps.back().initComputeStep();

    m_computeSteps.emplace_back(m_context, m_numSwapChainImages);
    m_computeSteps.back().setName("Module Voxel Write");
    m_computeSteps.back().createShaderModules(
        {"plants/moduleVoxelWrite.comp"},
        m_descriptorSets, sceneCounts);
    m_computeSteps.back().setComputeMode(computeSimple, m_scene->getMaxPlantModules());
    m_computeSteps.back().initComputeStep();

    m_computeSteps.emplace_back(m_context, m_numSwapChainImages);
    m_computeSteps.back().setName("Accumulate Module Resources");
    m_computeSteps.back().createShaderModules(
        {"plants/accumulateModuleResources.comp"},
        m_descriptorSets, sceneCounts);
    m_computeSteps.back().setComputeMode(computeIterated, m_scene->getMaxPlantModules());
    m_computeSteps.back().setNumIterations(m_scene->getMaxModuleOrder()+1);
    m_computeSteps.back().initComputeStep();

    m_computeSteps.emplace_back(m_context, m_numSwapChainImages);
    m_computeSteps.back().setName("Distribute Module Resources");
    m_computeSteps.back().createShaderModules(
        {"plants/distributeModuleResources.comp"},
        m_descriptorSets, sceneCounts);
    m_computeSteps.back().setComputeMode(computeIterated, m_scene->getMaxPlantModules());
    m_computeSteps.back().setNumIterations(m_scene->getMaxModuleOrder()+1);
    m_computeSteps.back().initComputeStep();

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

    /*
    m_renderSteps.emplace_back(m_context, m_numSwapChainImages);
    m_renderSteps.back().setName("Visualize Voxels");
    m_renderSteps.back().createShaderModules(
        {"plants/visualizeVoxel.vert", "plants/visualizeVoxel.geom", "plants/visualizeVoxel.frag"},
        m_descriptorSets, sceneCounts);
    m_renderSteps.back().setRenderMode(renderInstancedPoint, m_numVoxels.x * m_numVoxels.y * m_numVoxels.z);
    m_renderSteps.back().setPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    m_renderSteps.back().initRenderStep(m_renderOutput[0], 0);
    */
}

void PlantRenderer::constructModuleGeometry() {
    std::cout << "   PLANT RENDERER: Contructing geometry" << std::endl;
    std::vector<std::shared_ptr<Mesh>> meshes;
    for(uint32_t p=0; p<m_scene->getNumPlantSpecies(); p++) {
        meshes.emplace_back(std::make_shared<Mesh>());
    }
    
    uint32_t maxModules = m_scene->getMaxPlantModules();
    std::vector<Module> modules(maxModules);
    uint32_t frameIndex = m_currentFrame % m_numSwapChainImages;
    
    VkBuffer tmpBuffer;
    VkDeviceMemory tmpMemory;
    VkDeviceSize bufferSize = maxModules * sizeof(Module);
    m_context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tmpBuffer, tmpMemory);
    m_context->copyBuffer(m_descriptorSets[2].getBuffer("PlantModules", frameIndex), tmpBuffer, bufferSize);
    void* moduleData;
    vkMapMemory(m_context->getDevice(), tmpMemory, 0, bufferSize, 0, &moduleData);
    memcpy(modules.data(), moduleData, bufferSize);

    for(auto &module : modules) {
        if(module.status > 0) {
            m_scene->getPlantSpecies(module.speciesIndex)->addModuleToMesh(module, meshes[module.speciesIndex]);
        }
    }
    
    vkUnmapMemory(m_context->getDevice(), tmpMemory);
    vkDestroyBuffer(m_context->getDevice(), tmpBuffer, nullptr);
    vkFreeMemory(m_context->getDevice(), tmpMemory, nullptr);

    //TO DO: clear buffer
}