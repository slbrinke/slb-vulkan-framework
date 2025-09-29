#ifndef SLBVULKAN_RENDERER_H
#define SLBVULKAN_RENDERER_H

#include <limits>

#include <glm/glm.hpp>

#include "Context.h"
#include "RenderOutput.h"
#include "RenderStep.h"

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
     */
    Renderer(std::shared_ptr<Context> &context);
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

    VkExtent2D m_imageExtent{0, 0}; /**< Size of the screen in number of pixels */
    VkViewport m_viewport{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}; /**< Viewport definition to render to a section of the screen */
    VkRect2D m_scissor{0, 0, {0, 0}}; /**< Scissor region to restrict the rendering area */

    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE; /**< Vulkan swap chain to render and display images in parallel */
    uint32_t m_numSwapChainImages = 2; /**< Number of images alternating in the swap chain */
    VkFormat m_swapChainFormat = VK_FORMAT_R8G8B8A8_SRGB; /**< Color format used for swap chain images */
    VkFormat m_depthFormat; /**< Format suitable for depth buffers */

    std::vector<RenderOutput> m_renderOutput; /**< List of output image sets to render to */
    std::vector<RenderStep> m_renderSteps; /**< Individual rendering steps iterated for every frame */

    Mesh m_testMesh;

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
     * TO DO: this is just a placeholder for now
     */
    void compute();

    /**
     * Gather all compute commands into the current compute command buffer.
     * 
     * Iterates over all compute steps and collects the necessary commands.
     */
    void recordComputeCommandBuffer();

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

    uint32_t m_currentFrame = 0; /**< Index of the current frame throughout the runtime */
    uint32_t m_swapChainImageIndex = 0; /**< Index of the current swap chain image */
    bool m_frameBufferResized = false;
    
};

/**
 * Simple test renderer implementing the Renderer baseclass.
 */
class SimpleRenderer : public Renderer {
public:
    SimpleRenderer(std::shared_ptr<Context> &context);
    ~SimpleRenderer() = default;

private:
    void setUpRenderOutput() override;
    void setUpRenderSteps() override;

};

#endif //SLBVULKAN_RENDERER_H