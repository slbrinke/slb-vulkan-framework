#include "Scene.h"

Scene::Scene() {
    m_rootNode = std::make_unique<SceneNode>();
}

glm::vec3 Scene::getBackgroundColor() {
    return m_backgroundColor;
}

void Scene::addSceneNode(std::unique_ptr<SceneNode> &sceneNode) {
    m_rootNode->addChild(sceneNode);
}

void Scene::init(std::shared_ptr<Context> &context, std::vector<DescriptorSet> &descriptorSets) {
    initSceneNode(context, m_rootNode);

    descriptorSets[1].addBuffer("Materials", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_numMaterials * sizeof(MaterialUniforms), false, nullptr);
}

void Scene::initSceneNode(std::shared_ptr<Context> &context, std::unique_ptr<SceneNode> &sceneNode) {
    if(sceneNode->hasMesh()) {
        if(!sceneNode->getMesh()->hasBuffers()) {
            sceneNode->getMesh()->createBuffers(context);
        }

        auto &mat = sceneNode->getMaterial();
        if(!mat->hasIndex()) {
            m_materialUniforms.emplace_back(mat->getUniformData());

            mat->setIndex(m_numMaterials);
            m_numMaterials++;
        }
    }

    for(auto &child : sceneNode->getChildren()) {
        initSceneNode(context, child);
    }
}

void Scene::updateUniforms(std::vector<DescriptorSet> &descriptorSets, uint32_t frameIndex) {
    descriptorSets[1].updateBuffer("Materials", frameIndex, m_materialUniforms.data());
}

void Scene::renderMeshes(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t numInstances) {
    m_rootNode->renderMesh(commandBuffer, pipelineLayout, numInstances);
}

void Scene::cleanUp(std::shared_ptr<Context> &context) {
    m_rootNode->cleanUp(context);
}