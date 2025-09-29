#include "Scene.h"

Scene::Scene() {
    m_meshes.emplace_back(std::make_shared<Mesh>());
}

void Scene::init(std::shared_ptr<Context> &context) {
    for(auto &mesh : m_meshes) {
        mesh->createBuffers(context);
    }
}

void Scene::renderMeshes(VkCommandBuffer commandBuffer, uint32_t numInstances) {
    for(auto &mesh : m_meshes) {
        mesh->render(commandBuffer, numInstances);
    }
}

void Scene::cleanUp(std::shared_ptr<Context> &context) {
    for(auto &mesh : m_meshes) {
        mesh->cleanUp(context);
    }
}