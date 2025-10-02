#include "Scene.h"

Scene::Scene() {
    m_rootNode = std::make_unique<SceneNode>();

    //screen quad
    m_defaultMeshes.emplace_back(std::make_shared<Mesh>());
    m_defaultMeshes[0]->addVertex(
        glm::vec3(-1.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    m_defaultMeshes[0]->addVertex(
        glm::vec3(1.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    m_defaultMeshes[0]->addVertex(
        glm::vec3(1.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    m_defaultMeshes[0]->addVertex(
        glm::vec3(-1.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    m_defaultMeshes[0]->addIndex(0);
    m_defaultMeshes[0]->addIndex(1);
    m_defaultMeshes[0]->addIndex(2);
    m_defaultMeshes[0]->addIndex(2);
    m_defaultMeshes[0]->addIndex(3);
    m_defaultMeshes[0]->addIndex(0);
    //point light sphere
    m_defaultMeshes.emplace_back(std::make_shared<Mesh>());
    m_defaultMeshes[1]->addSphere(glm::vec3(0.0f), 1.0f, 10);
    //spot light cone
    m_defaultMeshes.emplace_back(std::make_shared<Mesh>());
    m_defaultMeshes[2]->addCone(glm::vec3(0.0f, -1.0f, 0.0f), 1.0f, 1.0f, 10);
    
}

glm::vec3 Scene::getBackgroundColor() {
    return m_backgroundColor;
}

std::vector<uint32_t> Scene::getSceneCounts() {
    std::vector<uint32_t> counts = {m_numMaterials, m_numLights, m_numTextures};
    return counts;
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

    descriptorSets[1].addBuffer("Materials", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_numMaterials * sizeof(MaterialUniforms), false, nullptr);
    descriptorSets[1].addBuffer("Lights", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_numLights * sizeof(LightUniforms), false, nullptr);
    std::vector<uint32_t> sceneCounts = {m_numMaterials, m_numLights,m_numTextures};
    descriptorSets[1].addBuffer("SceneCounts", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sceneCounts.size() * sizeof(uint32_t), false, sceneCounts.data());

    std::vector<VkImageView> textureImageViews;
    for(auto &texture : m_textures) {
        textureImageViews.emplace_back(texture.getView());
    }
    descriptorSets[1].addImages(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textureImageViews);

    for(auto &mesh : m_defaultMeshes) {
        mesh->createBuffers(context);
    }
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

            if(mat->hasDiffuseTexture()) {
                m_textures.emplace_back(context, mat->getDiffuseTexture());
                m_materialUniforms[m_numMaterials].diffuseTextureIndex = m_numTextures;
                m_numTextures++;
            }
            if(mat->hasNormalTexture()) {
                m_textures.emplace_back(context, mat->getNormalTexture());
                m_materialUniforms[m_numMaterials].normalTextureIndex = m_numTextures;
                m_numTextures++;
            }
            if(mat->hasRoughnessTexture()) {
                m_textures.emplace_back(context, mat->getRoughnessTexture());
                m_materialUniforms[m_numMaterials].roughnessTextureIndex = m_numTextures;
                m_numTextures++;
            }
            if(mat->hasMetallicTexture()) {
                m_textures.emplace_back(context, mat->getMetallicTexture());
                m_materialUniforms[m_numMaterials].metallicTextureIndex = m_numTextures;
                m_numTextures++;
            }

            mat->setIndex(m_numMaterials);
            m_numMaterials++;
        }
    }

    if(sceneNode->hasLight()) {
        auto &light = sceneNode->getLight();
        m_lightUniforms.emplace_back(light->getUniformData(model));
        light->setIndex(m_numLights);
        m_numLights++;

        if(light->isDirectionalLight()) {
            light->setProxyMesh(m_defaultMeshes[0]);
        } else if(light->isPointLight()) {
            light->setProxyMesh(m_defaultMeshes[1]);
        } else { //spot light
            light->setProxyMesh(m_defaultMeshes[2]);
        }
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

void Scene::renderScreenQuad(VkCommandBuffer commandBuffer) {
    m_defaultMeshes[0]->render(commandBuffer, 1);
}

void Scene::renderLightProxies(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    m_rootNode->renderLightProxy(commandBuffer, pipelineLayout);
}

void Scene::cleanUp(std::shared_ptr<Context> &context) {
    m_rootNode->cleanUp(context);

    for(auto &texture : m_textures) {
        texture.cleanUp(context);
    }

    for(auto &mesh : m_defaultMeshes) {
        mesh->cleanUp(context);
    }
}