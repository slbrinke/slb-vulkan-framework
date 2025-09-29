#ifndef SLBVULKAN_SCENE_H
#define SLBVULKAN_SCENE_H

#include "Mesh.h"

class Scene {
public:
    Scene();
    ~Scene() = default;

    void init(std::shared_ptr<Context> &context);

    void renderMeshes(VkCommandBuffer commandBuffer, uint32_t numInstances = 1);

    void cleanUp(std::shared_ptr<Context> &context);
private:
    std::vector<std::shared_ptr<Mesh>> m_meshes;

};

#endif //SLBVULKAN_SCENE_H