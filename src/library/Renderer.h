#ifndef SLBVULKAN_RENDERER_H
#define SLBVULKAN_RENDERER_H

#include <limits>

#include <glm/glm.hpp>

#include "Context.h"
#include "Scene.h"
#include "RenderOutput.h"
#include "Step.h"

/**
 * Uniform data providing constants used for different purposes in the shaders.
 */
struct RendererUniforms {
    uint32_t useEnvMap; /**< 1 (=true) if an environment map is added to the scene */
    float envMapOffset; /**< Horizontal offset of the environment map */
    uint32_t renderShadows; /**< 1 (=true) if shadow mapping is enabled */
    uint32_t shadowMapSize; /**< Width and height of the shadow maps in number of pixels */
    float pi; /**< The mathematical constant pi */
    float inversePi; /**< One divided by pi */
    float epsilon; /**< Very small value */
    float pad;
};

/**
 * Uniform data providing scene properties relevant for simulation in the shaders.
 * 
 * This includes a simulation timer and voxel grid sizes.
 */
struct SimulationUniforms {
    glm::vec3 sceneSize; /**< Total size of the scene in world coordinates */
    float currTime; /**< Time passed since the start of the simulation in seconds */
    glm::ivec3 numVoxels; /**< Number of voxels in each dimension */
    float deltaTime; /**< Time passed since the last frame in seconds */
    glm::vec3 voxelSize; /**< Size of a single voxel in each dimension */
    uint32_t maxCandidates;
};

/**
 * Renderer baseclass containing basic rendering functionality.
 * 
 * The subclasses implement different rendering strategies by defining custom render output and render steps.
 * Components handled by the base renderer include swap chain, and synchronization objects.
 */
class Renderer {
public:
    /**
     * Create the base for a renderer.
     * 
     * A new smart pointer to the vulkan context is stored for later use.
     * Includes the creation of a swap chain that can be used in the output defined by the subclasses.
     * 
     * @param context pointer to the vulkan context
     * @param scene pointer to the scene that will be visualized
     */
    Renderer(std::shared_ptr<Context> &context, std::shared_ptr<Scene> &scene);
    ~Renderer();

    /**
     * Update relevant data and optional compute simulation.
     */
    void update();

    /**
     * Render scene contents into the defined output images.
     */
    void render();

    /**
     * Destroy all vulkan components.
     * 
     * Sync objects, render steps, render output, and swap chain are destroyed in reverse order of creation.
     * Command buffer are not destroyed, because the command pool will be destroyed in context.cleanUp.
     */
    void cleanUp();

protected:
    /**
     * Set up output to render to.
     * 
     * Has to be implemented in the subclasses.
     * Swap chain should be integrated into RenderOutput via addSwapChainAttachment.
     */
    virtual void setUpRenderOutput();

    /**
     * Set up descriptor sets to provide data required in the shaders.
     */
    void setUpDescriptorSets();

    /**
     * Set up individual render steps.
     * 
     * Has to be implemented in the subclasses.
     * Render steps can use different shaders, render to different output images, and have additional settings.
     */
    virtual void setUpRenderSteps();

    /**
     * Create command buffers for graphics and compute commands.
     */
    void createCommandBuffers();

    /**
     * Create synchronization objects.
     */
    void createSyncObjects();

    std::shared_ptr<Context> m_context; /**< Pointer to the vulkan context */
    std::shared_ptr<Scene> m_scene; /**< Pointer to the rendered scene */

    VkExtent2D m_imageExtent{0, 0}; /**< Size of the screen in number of pixels */
    VkViewport m_viewport{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}; /**< Viewport definition to render to a section of the screen */
    VkRect2D m_scissor{0, 0, {0, 0}}; /**< Scissor region to restrict the rendering area */

    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE; /**< Vulkan swap chain to render and display images in parallel */
    uint32_t m_numSwapChainImages = 2; /**< Number of images alternating in the swap chain */
    VkFormat m_swapChainFormat = VK_FORMAT_R8G8B8A8_SRGB; /**< Color format used for swap chain images */
    VkFormat m_depthFormat; /**< Format suitable for depth buffers */

    uint32_t m_currentFrame = 0; /**< Index of the current frame throughout the runtime */

    std::vector<DescriptorSet> m_descriptorSets; /**< List of descriptor sets added to render steps as requested in the shaders */
    std::vector<RenderOutput> m_renderOutput; /**< List of output image sets to render to */
    std::vector<RenderStep> m_renderSteps; /**< Individual rendering steps iterated for every frame */
    std::vector<ComputeStep> m_computeSteps; /**< Individual compute steps iterated for every frame */

    bool m_renderShadows = true; /**< State parameter turning shadow mapping on and off */
    uint32_t m_shadowMapSize = 1024; /**< Size of all generated shadow maps in number of pixels */

    std::shared_ptr<Mesh> m_pointMesh = nullptr; /**< Dummy mesh with a single point used to instantiate shader storage data */

    float m_prevTime = 0.0f; /**< Runtime passed until the previous frame in seconds */
    float m_currTime = 0.0f; /**< Runtime passed until the current frame in seconds */

    bool m_useVoxels = false; /**< State parameter turning voxelization on and off */
    float m_avgVoxelSize = 2.0f; /**< Ideal size of a single voxel in world space (used to determine number of voxels) */
    glm::ivec3 m_numVoxels{4}; /**< Number of voxels in each dimension (has to be a power of 2) */
    uint32_t m_numVoxelCandidates = 0; /**< Average number of elements that can be stored per voxel (used to determine voxel buffer size) */

private:
    /**
     * Create a swap chain from the vulkan context.
     * 
     * Screen size, viewport, and scissor are adapted to the generated swap chain images.
     */
    void createSwapChain();

    /**
     * Execute compute steps.
     * 
     * If compute steps have been defined all relevant commands are recorded and passed to the queue.
     */
    void compute();

    /**
     * Gather all compute commands into the current compute command buffer.
     * 
     * Iterates over all compute steps and collects the necessary commands.
     */
    void recordComputeCommandBuffer();

    /**
     * Simple compute call recorded for the currently active compute step.
     * 
     * @param numInvocations number of invocations of the active compute shader
     */
    void dispatchComputeSimple(uint32_t numInvocations);

    /**
     * Multiple compute call recorded for the currently active compute step.
     * 
     * The index of the current iteration is passed as a push constant.
     * 
     * @param numIterations number of compute dispatches
     * @param numInvocations number of invocations for each compute dispatch
     * @param pipelineLayout pipeline layout of the current render step
     */
    void dispatchComputeIterated(uint32_t numIterations, uint32_t numInvocations, VkPipelineLayout pipelineLayout);

    /**
     * Cascaded compute calls recorded for the currently active compute step.
     * 
     * Iterative dispatch computes with changing work group size.
     * Can be used for prefix sum calculation during voxelization.
     * 
     * @param numIterations number of dispatch iterations
     * @param initialWorkGroup initial size of the work groups in the first iteration
     * @param workGroupFactor increase/decrease of the work group size between iterations
     * @param push dictates whether the work group size decreases or increases
     */
    void dispatchComputeCascaded(uint32_t numIterations, uint32_t initialWorkGroup, uint32_t workGroupFactor, bool push);

    /**
     * Gather all graphics commands into the current graphics command buffer.
     * 
     * Iterates over all render steps and collects the necessary commands.
     * Enables and disables the relevant outputs along the way.
     */
    void recordGraphicsCommandBuffer();

    //command buffers
    std::vector<VkCommandBuffer> m_graphicsCommandBuffers; /**< Command buffers for graphics commands for each swap chain image */
    std::vector<VkCommandBuffer> m_computeCommandBuffers; /**< Command buffers for compute commands for each swap chain image */

    //sync objects
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_computeFinishedSemaphores;
    std::vector<VkSemaphore> m_graphicsFinishedSemaphores;
    std::vector<VkFence> m_computeInFlightFences;
    std::vector<VkFence> m_graphicsInFlightFences;

    uint32_t m_swapChainImageIndex = 0; /**< Index of the current swap chain image */
    bool m_frameBufferResized = false;
    
};

#endif //SLBVULKAN_RENDERER_H