#ifndef SLBVULKAN_SCENENODE_H
#define SLBVULKAN_SCENENODE_H

#include "Mesh.h"
#include "Material.h"
#include "Light.h"

/**
 * Temporary rendering information associated with the current scene node.
 * 
 * Used to communicate scene node information to shaders as push constants.
 */
struct SceneNodeConstants {
    glm::mat4 model;
    uint32_t materialIndex;
};

/**
 * Node serving as an individual element of the scene graph hierarchy.
 * 
 * Places a mesh and/or light source in the scene with specified position, rotation and scale.
 * The transformations are passed on to a list of child nodes.
 */
class SceneNode {
public:
    /**
     * Create a default scene node.
     * It contains no mesh or light source yet.
     * Transformation is initialized with position (0, 0, 0), scale(1, 1, 1) and no rotation.
     */
    SceneNode();

    /**
     * Create a scene node instantiating a given mesh in the scene.
     * 
     * @param mesh pointer to the mesh rendered by this node
     * @param material pointer to the material applied to the mesh
     */
    SceneNode(std::shared_ptr<Mesh> &mesh, std::shared_ptr<Material> &material);
    ~SceneNode();

    /**
     * Return the model matrix transforming the local coordinates of the node into the coordinates of the parent node.
     */
    glm::mat4 getModelMatrix();

    /**
     * Check if the scene node contains a mesh.
     */
    bool hasMesh();

    /**
     * Return a pointer to the mesh attached to the scene node.
     * 
     * Meshes are stored in shared pointers because multiple nodes can instantiate the same mesh.
     */
    std::shared_ptr<Mesh> &getMesh();

    /**
     * Return a pointer to the material applied to the attached mesh.
     */
    std::shared_ptr<Material> &getMaterial();

    /**
     * Check if the scene node contains a light source.
     */
    bool hasLight();

    /**
     * Return a pointer to the light source attached to the scene node.
     * 
     * Lights are stored in a unique pointer because the uniform information is unique for each light source.
     * When retrieving e.g. the world position of a light source in a shader multiple instances of the same light would return the same data otherwise.
     */
    std::unique_ptr<Light> &getLight();

    /**
     * Provide references to the list of children within the scene graph hierarchy.
     * @return List of pointers to the child scene nodes.
     */
    std::vector<std::unique_ptr<SceneNode>> &getChildren();

    /**
     * Change the origin of the local coordinates of the scene node.
     * 
     * @param position new local coordinate origin
     */
    void setPosition(glm::vec3 position);

    /**
     * Rotate the local coordinate system of the scene node.
     * 
     * The rotation does not replace the scene node rotation but is applied to it.
     * 
     * @param degrees angle of the rotation in degrees
     * @param axis rotation axis
     */
    void rotate(float degrees, glm::vec3 axis);

    /**
     * Change the scale of the local coordinate system of the scene node.
     * 
     * This effectively changes the size of the contained mesh.
     * 
     * @param scale factor scaling the local coordinate system
     */
    void setScale(float scale);

    /**
     * Scale the local coordinate system of the scene node.
     * 
     * Instead of replacing the scale, as setScale does, the factors are multiplied.
     * 
     * @param scale factor scaling the local coordinate system
     */
    void scale(float scale);

    /**
     * Add a mesh to be displayed with the transformation of the scene node.
     * 
     * A material has to be supplied for rendering.
     * 
     * @param mesh pointer to the added mesh
     * @param material pointer to the material applied to the mesh
     */
    void addMesh(std::shared_ptr<Mesh> &mesh, std::shared_ptr<Material> &material);

    /**
     * Add a light source to the scene node.
     * 
     * Light sources cannot be instanced like meshes or materials.
     * To copy a light to a different position a new light source has to be created.
     * Since the new light is stored in a unique pointer the pointer will be empty after.
     * 
     * @param pointer to the new light source
     */
    void addLight(std::unique_ptr<Light> &light);

    /**
     * Add a scene node into the scene graph hierarchy below this node.
     * 
     * All local transformations will affect the new child node.
     * Since the new node is stored in a unique pointer the pointer will be empty after.
     * 
     * @param child Unique pointer to the scene node added as a child.
     */
    void addChild(std::unique_ptr<SceneNode> &child);

    /**
     * Render the attached mesh.
     * 
     * Recursively called for all child nodes.
     * 
     * @param commandBuffer graphics command buffer receiving the draw command
     * @param pipelineLayout pipeline layout of the current render step
     * @param numInstances number of instances rendered for the mesh
     * @param parentModel model matrix of the parent node
     */
    void renderMesh(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t numInstances, glm::mat4 parentModel = glm::mat4(1.0f));

    /**
     * Destroy all vulkan components.
     * 
     * Calls cleanUp for the attached mesh.
     */
    void cleanUp(std::shared_ptr<Context> &context);

private:
    glm::vec3 m_position{0.0f}; /**< Position the node is located at in the local coordinates of the parent node */
    glm::quat m_rotation{1.0f, 0.0f, 0.0f, 0.0f}; /**< Rotation of the node in the local coordinates of the parent node */
    float m_scale{1.0f}; /**< Factor scaling the node contents relative to the parent node */  
    
    std::shared_ptr<Mesh> m_mesh = nullptr; /**< Geometry rendered with the transformations of this scene node */
    std::shared_ptr<Material> m_material = nullptr; /**< Material applied to the mesh instance */

    std::unique_ptr<Light> m_light = nullptr; /**< Light source emitting light into the scene */

    std::vector<std::unique_ptr<SceneNode>> m_children; /**< Child nodes subject to this node's transformations */

};

#endif //SLBVULKAN_SCENENODE_H