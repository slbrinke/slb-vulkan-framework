#ifndef SLBVULKAN_SCENE_H
#define SLBVULKAN_SCENE_H

#include "Camera.h"
#include "ResourceLoader.h"
#include "DescriptorSet.h"
#include "Image.h"
#include "Light.h"
#include "PlantSpecies.h"

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
     * 
     * @param camera pointer to the camera the scene will be seen through
     */
    Scene(std::shared_ptr<Camera> &camera);
    ~Scene() = default;

    /**
     * Return the total size of the space occupied by elements of the scene.
     * 
     * The center is assumed to be at world origin.
     * 
     * @return scene size in world coordinates
     */
    glm::vec3 getSize();

    /**
     * Return the background color the scene is displayed in front of.
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
     * Return the maximum number of plant modules in the scene.
     * 
     * @return size of the plant module buffer
     */
    uint32_t getMaxPlantModules();

    /**
     * Return the maximum module order among all plant species.
     * 
     * @return maximum tree depth in number of modules
     */
    uint32_t getMaxModuleOrder();

    /**
     * Return the maximum number of branch segments in the scene.
     * 
     * It is an upper bound estimated from the maximum number of nodes in a prototype and the maximum number of plant modules.
     * 
     * @return estimated branch segment number
     */
    uint32_t getNumBranches();

    uint32_t getNumPlantSpecies();

    std::unique_ptr<PlantSpecies> &getPlantSpecies(uint32_t speciesIndex);

    std::vector<Prototype> &getPlantPrototypes();

    /**
     * Change the total size of the scene.
     * 
     * The center is assumed to be at world origin.
     * This overrides the value updated based on the meshes placed in the scene.
     * 
     * @param size scene size in world coordinates
     */
    void setSize(glm::vec3 size);

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
     * Add an hdri image as environment map.
     * 
     * @param fileName name of an image file in the resources/textures folder
     */
    void addEnvironmentMap(std::string fileName);

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
     * Add a number of plants of a new species.
     * 
     * The exact plants must be added to the species before this point.
     * 
     * @param plantSpecies added plant species
     */
    void addPlants(std::unique_ptr<PlantSpecies> &plantSpecies);

    /**
     * Initialize meshes, materials, and descriptor sets.
     * 
     * Mesh buffers are created and material uniforms are gathered to be provided via descriptor sets.
     * This has to be called before the scene can be rendered.
     * No new meshes or materials can be added to the scene after this point.
     * 
     * If plant species have been added species and protoype uniforms and module buffer are added.
     * 
     * @param context pointer to the vulkan context
     * @param descriptorSets list of all descriptor sets used by a renderer
     */
    void init(std::shared_ptr<Context> &context, std::vector<DescriptorSet> &descriptorSets);

    /**
     * Update uniform data at the beginning of a new frame.
     * 
     * This includes camera and material uniforms.
     * For the light uniforms of directional light sources the shadow map transformations are updated.
     * Plant species and module prototypes are also updated.
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
     * Record the draw command for the environment map.
     * 
     * The predetermined hdri image is displayed on a sphere enveloping the scene.
     * 
     * @param commandBuffer graphics command buffer receiving the draw command
     */
    void renderEnvironmentMap(VkCommandBuffer commandBuffer);

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

    std::shared_ptr<Camera> m_camera; /**< Pointer to the camera viewing the scene */

    glm::vec3 m_size{1.0f}; /**< Size of the total space occupied in world coordinates */
    glm::vec3 m_backgroundColor{0.43f, 0.38f, 0.3f}; /**< Color displayed in the background of the scene */

    std::unique_ptr<SceneNode> m_rootNode; /**< Root node of the scene graph */

    uint32_t m_numMaterials = 0; /**< Number of materials applied throughout the scene graph */
    std::vector<MaterialUniforms> m_materialUniforms; /**< Uniform data for all materials in the scene */
    uint32_t m_numTextures = 0; /**< Number of textures attached to the materials */
    std::vector<Image> m_textures; /**< Texture images required by the materials */

    bool m_hasEnvMap = false; /**< Stores whether an environment map has been assigned */
    std::string m_envMapFile = ""; /**< Name of the image file containing the hdri environment map */

    uint32_t m_numLights = 0; /**< Number of light sources in the scene graph */
    std::vector<LightUniforms> m_lightUniforms; /**< Uniform data for all lights in the scene */

    std::vector<std::shared_ptr<Mesh>> m_defaultMeshes; /**< Default meshes required for deferred rendering */

    uint32_t m_numNodes = 0; /**< Number of nodes in the scene */
    std::vector<Node> m_nodes; /**< List of nodes provided to shaders */
    uint32_t m_maxNodes = 30; /**< Size of the node buffer */

    //plants
    uint32_t m_numPlantSpecies = 0; /**< Number of plant species */
    std::vector<std::unique_ptr<PlantSpecies>> m_plantSpecies;
    std::vector<SpeciesUniforms> m_speciesUniforms; /**< Uniform data for all plant species in the scene */
    uint32_t m_numPlantPrototypes = 0; /**< Number of plant module prototypes */
    std::vector<Prototype> m_plantPrototypes; /**< Uniform data for the module prototypes of all plant species */
    uint32_t m_numPlantModules = 0; /**< Number of initialized plant modules of all plant species */
    std::vector<Module> m_plantModules; /**< List of plant modules for all plant species */
    uint32_t m_maxPlantModules = 30; /**< Size of the plant module buffer */
    uint32_t m_maxModuleOrder = 0; /**< Maximum plant hierarchy depth in number of modules over all defined species */
    uint32_t m_maxNodesPerModule = 0; /**< Maximum number of nodes contained in any of the plant module prototypes */

};

#endif //SLBVULKAN_SCENE_H