#include "SceneNode.h"

SceneNode::SceneNode() {

}

SceneNode::SceneNode(std::shared_ptr<Mesh> &mesh, std::shared_ptr<Material> &material) {
    addMesh(mesh, material);
}

SceneNode::~SceneNode() {
    if(m_mesh != nullptr) {
        m_mesh = nullptr;
        m_material = nullptr;
    }
}

glm::mat4 SceneNode::getModelMatrix() {
    return glm::scale(glm::translate(glm::mat4(1.0f), m_position) * glm::mat4_cast(m_rotation), glm::vec3(m_scale));
}

bool SceneNode::hasMesh() {
    return m_mesh != nullptr;
}

std::shared_ptr<Mesh> &SceneNode::getMesh() {
    return m_mesh;
}

std::shared_ptr<Material> &SceneNode::getMaterial() {
    return m_material;
}

bool SceneNode::hasLight() {
    return m_light != nullptr;
}

std::unique_ptr<Light> &SceneNode::getLight() {
    return m_light;
}

std::vector<std::unique_ptr<SceneNode>> &SceneNode::getChildren() {
    return m_children;
}

void SceneNode::setPosition(glm::vec3 position) {
    m_position = position;
}

void SceneNode::rotate(float degrees, glm::vec3 axis) {
    m_rotation = glm::rotate(m_rotation, glm::radians(degrees), axis);
}

void SceneNode::setScale(float scale) {
    m_scale = scale;
}

void SceneNode::scale(float scale) {
    m_scale *= scale;
}

void SceneNode::addMesh(std::shared_ptr<Mesh> &mesh, std::shared_ptr<Material> &material) {
    m_mesh = mesh;
    m_material = material;
}

void SceneNode::addLight(std::unique_ptr<Light> &light) {
    m_light = std::move(light);
}

void SceneNode::addChild(std::unique_ptr<SceneNode> &child) {
    m_children.emplace_back(std::move(child));
}

void SceneNode::renderMesh(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t numInstances, glm::mat4 parentModel) {
    auto model = parentModel * getModelMatrix();

    if(m_mesh != nullptr) {
        SceneNodeConstants constants {
            model,
            m_material->getIndex()
        };
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SceneNodeConstants), &constants);

        m_mesh->render(commandBuffer, numInstances);
    }

    for(auto &child : m_children) {
        child->renderMesh(commandBuffer, pipelineLayout, numInstances, model);
    }
}

void SceneNode::cleanUp(std::shared_ptr<Context> &context) {
    if(m_mesh != nullptr && m_mesh->hasBuffers()) {
        m_mesh->cleanUp(context);
    }

    for(auto &child : m_children) {
        child->cleanUp(context);
    }
}