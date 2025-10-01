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

void Scene::addSun(float theta, float phi, glm::vec3 color, float intensity) {
    auto sunDirection = glm::vec3(
        glm::sin(glm::radians(phi)) * glm::cos(glm::radians(theta)),
        glm::sin(glm::radians(theta)),
        glm::cos(glm::radians(phi)) * glm::cos(glm::radians(theta))
    );
    auto sun = std::make_unique<Light>(glm::vec3(0.0f), -sunDirection);
    sun->setSpotAngle(0.0f);
    sun->setColor(color);
    sun->setIntensity(intensity);
    m_rootNode->addLight(sun);
}

void Scene::init(std::shared_ptr<Context> &context, std::vector<DescriptorSet> &descriptorSets) {
    initSceneNode(context, m_rootNode);

    if(m_materialUniforms.size() > MAX_MATERIALS) {
        throw std::runtime_error("SCENE ERROR: Can only contain " + std::to_string(MAX_MATERIALS) + " materials.");
    }
    if(m_materialUniforms.size() < MAX_MATERIALS) {
        m_materialUniforms.resize(MAX_MATERIALS);
    }
    descriptorSets[1].addBuffer("Materials", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_MATERIALS * sizeof(MaterialUniforms), false, nullptr);
    
    if(m_lightUniforms.size() > MAX_LIGHTS) {
        throw std::runtime_error("SCENE ERROR: Can only contain " + std::to_string(MAX_LIGHTS) + " light sources.");
    }
    if(m_lightUniforms.size() < MAX_LIGHTS) {
        m_lightUniforms.resize(MAX_LIGHTS);
    }
    descriptorSets[1].addBuffer("Lights", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_LIGHTS * sizeof(LightUniforms), false, nullptr);
}

void Scene::initSceneNode(std::shared_ptr<Context> &context, std::unique_ptr<SceneNode> &sceneNode, glm::mat4 parentModel) {
    auto model = parentModel * sceneNode->getModelMatrix();

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

    if(sceneNode->hasLight()) {
        auto &light = sceneNode->getLight();
        m_lightUniforms.emplace_back(light->getUniformData(model));
        light->setIndex(m_numLights);
        m_numLights++;
    }

    for(auto &child : sceneNode->getChildren()) {
        initSceneNode(context, child, model);
    }
}

void Scene::updateUniforms(std::vector<DescriptorSet> &descriptorSets, uint32_t frameIndex) {
    descriptorSets[1].updateBuffer("Materials", frameIndex, m_materialUniforms.data());
    descriptorSets[1].updateBuffer("Lights", frameIndex, m_lightUniforms.data());
}

void Scene::renderMeshes(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t numInstances) {
    m_rootNode->renderMesh(commandBuffer, pipelineLayout, numInstances);
}

void Scene::cleanUp(std::shared_ptr<Context> &context) {
    m_rootNode->cleanUp(context);
}