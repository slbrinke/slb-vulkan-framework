#ifndef SLBVULKAN_SCENE_H
#define SLBVULKAN_SCENE_H

#include "ResourceLoader.h"
#include "DescriptorSet.h"
#include "Image.h"
#include "Light.h"

/**
 * Three-dimensional scene defining geometry and surfaces.
 * 
 * Manages meshes and materials in a scene graph hierarchy.
 * Provides uniforms and draw calls for a renderer.
 */
class Scene {
public:
    /**
     * Create an empty scene.
     * 
     * Sets up default root node for the scene graph.
     */
    Scene();
    ~Scene() = default;

    /**
     * Change the background color the scene is displayed in front of.
     * 
     * @return background color in rgb format
     */
    glm::vec3 getBackgroundColor();

    /**
     * Return the total numbers of different components of the scene.
     * 
     * Including the number of materials, light sources, and textures extracted from the scene graph.
     */
    std::vector<uint32_t> getSceneCounts();

    /**
     * Add a new scene node to the scene graph.
     * 
     * The new node is added as a child to the root node.
     * Since it is stored in a unique pointer the pointer will be empty after.
     * 
     * @param sceneNode pointer to a new scene node
     */
    void addSceneNode(std::unique_ptr<SceneNode> &sceneNode);

    /**
     * Add the sun as a default light source.
     * 
     * The sun is represented by a directional light source added to the root node of the scene graph.
     * Where the sun is on the hemisphere is dictated by the angles theta and phi.
     * Both are given in degrees, theta has to be in range [-0.5*pi, 0.5*pi], and phi in range [0, 2*pi].
     * 
     * @param theta vertical angle in degrees
     * @param phi horizontal angle in degrees
     * @param color light color emitted by the sun
     * @param intensity light intensity emitted by the sun
     */
    void addSun(float theta, float phi, glm::vec3 color, float intensity);

    /**
     * Initialize meshes, materials, and descriptor sets.
     * 
     * Mesh buffers are created and material uniforms are gathered to be provided via descriptor sets.
     * This has to be called before the scene can be rendered.
     * No new meshes or materials can be added to the scene after this point.
     * 
     * @param context pointer to the vulkan context
     * @param descriptorSets list of all descriptor sets used by a renderer
     */
    void init(std::shared_ptr<Context> &context, std::vector<DescriptorSet> &descriptorSets);

    /**
     * Update material uniform data at the beginning of a new frame.
     * 
     * @param descriptorSets list of all descriptor sets used by a renderer
     * @param frameIndex index of the current frame in flight
     */
    void updateUniforms(std::vector<DescriptorSet> &descriptorSets, uint32_t frameIndex);

    /**
     * Record draw calls for all meshes in the scene graph.
     * 
     * @param commandBuffer graphics command buffer receiving the draw commands
     * @param pipelineLayout pipeline layout of the current render step
     * @param numInstances number of instances rendered for each mesh
     */
    void renderMeshes(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t numInstances = 1);

    /**
     * Record the draw command for a screen-aligned quad.
     * 
     * @param commandBuffer graphics command buffer receiving the draw command
     */
    void renderScreenQuad(VkCommandBuffer commandBuffer);

    /**
     * Record draw calls for the proxy geometry of each light source in the scene graph.
     * 
     * Serves as the second step for a deferred renderer.
     * 
     * @param commandBuffer graphics command buffer receiving the draw command
     * @param pipelineLayout pipeline layout of the current render step
     */
    void renderLightProxies(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    /**
     * Destroy all vulkan components.
     * 
     * Calls cleanUp function for all meshes in the scene graph.
     */
    void cleanUp(std::shared_ptr<Context> &context);
private:
    /**
     * Initialize the mesh and material in a given scene node.
     * 
     * Recursively called for all child nodes.
     * 
     * @param context pointer to the vulkan context
     * @param sceneNode node in the scene graph
     * @param parentModel model matrix of the parent node
     */
    void initSceneNode(std::shared_ptr<Context> &context, std::unique_ptr<SceneNode> &sceneNode, glm::mat4 parentModel = glm::mat4(1.0f));

    glm::vec3 m_backgroundColor{0.43f, 0.38f, 0.3f}; /**< Color displayed in the background of the scene */

    std::unique_ptr<SceneNode> m_rootNode; /**< Root node of the scene graph */

    uint32_t m_numMaterials = 0; /**< Number of materials applied throughout the scene graph */
    std::vector<MaterialUniforms> m_materialUniforms; /**< Uniform data for all materials in the scene */
    uint32_t m_numTextures = 0; /**< Number of textures attached to the materials */
    std::vector<Image> m_textures; /**< Texture images required by the materials */

    uint32_t m_numLights = 0; /**< Number of light sources in the scene graph */
    std::vector<LightUniforms> m_lightUniforms; /**< Uniform data for all lights in the scene */

    std::vector<std::shared_ptr<Mesh>> m_defaultMeshes; /**< Default meshes required for deferred rendering */

};

#endif //SLBVULKAN_SCENE_H