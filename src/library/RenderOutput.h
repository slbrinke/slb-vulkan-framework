#ifndef SLBVULKAN_RENDEROUTPUT_H
#define SLBVULKAN_RENDEROUTPUT_H

#include "Context.h"
#include "Image.h"

/**
 * Render attachment for a specific output image.
 * 
 * Every attachment has a main image.
 * If multisampling is enabled in the subpass the main image is multisampled and an additional resolve image is created.
 * The attachment stores the index/indices of the image/s in the m_images list.
 */
struct Attachment {
    uint32_t mainIndex = 0; /**< Index of the main image in the RenderOuput m_images list */
    bool useMultisampling = false; /**< If true the main image is multisampled */
    bool hasResolve = false; /**< If true the attachment has an additional resolve image */
    uint32_t resolveIndex = 0; /**< Index of the multisampling resole image in the RenderOutput m_images list */
};

/**
 * Subset of outputs arranged in a separate vulkan subpass.
 */
struct SubPass {
    uint32_t numAttachments = 0; /**< Number of attachments associated with the subpass */
    uint32_t firstAttachment = 0; /**< Index of the first attachment associated with the subpass */

    std::vector<std::pair<uint32_t, uint32_t>> subPassInputs;
    std::vector<std::pair<VkImageView,bool>> externalInputs;
    uint32_t descriptorSetIndex = 0;

    uint32_t numColorAttachments = 0; /**< Number of color output attachments */
    bool useDepth = false; /**< True if the subpass has a depth attachment */
    bool useMultisampling = false; /**< True if MSAA is used when the subpass is active */

    std::vector<VkClearAttachment> clearAttachments; /**< Clear values for all attachments in the subpass */
};

/**
 * Set of images a pipeline can render to.
 * 
 * Manages a vulkan renderpass and all attached images.
 */
class RenderOutput {
public:
    /**
     * Set up render output with an empty subpass.
     * 
     * A new smart pointer to the vulkan context is stored for later use.
     * 
     * @param context pointer to the vulkan context
     * @param numFramesInFlight number of images alternated in the swap chain
     * @param imageExtent size of all output images in number of pixels
     * @param numLayers number of layers of all output images
     * @param useMultisampling if true MSAA is used in the initial subpass
     */
    RenderOutput(std::shared_ptr<Context> &context, uint32_t numFramesInFlight, VkExtent2D imageExtent, uint32_t numLayers = 1, bool useMultisampling = false);
    ~RenderOutput();

    /**
     * Return the number of color attachments of a specific subpass.
     * 
     * @param subPassIndex index of the subpass within the render output
     * @return number of color attachments of the subpass
     */
    uint32_t getNumSubPassColorAttachments(uint32_t subPassIndex);

    /**
     * Check whether a specific subpass has a depth attachment.
     * 
     * @param subPassIndex index of the subpass within the render output
     * @return true if the subpass has a depth attachment
     */
    bool subPassUsesDepth(uint32_t subPassIndex);

    /**
     * Check whether a specific subpass contains multisampled images.
     * 
     * @param subPassIndex index of the subpass within the render output
     * @return true if the subpass attachments use multisampling
     */
    bool subPassUsesMultisampling(uint32_t subPassIndex);

    /**
     * Return the vulkan renderpass encompassing all output images.
     * 
     * @return vulkan handle of the renderpass
     */
    VkRenderPass getRenderPass();

    /**
     * Add a new subset of output images.
     * 
     * @param useMultisampling if true MSAA is used when rendering to the output images
     */
    void addSubPass(bool useMultisampling);

    /**
     * Add a color attachment to the most recently added subpass.
     * 
     * @param colorFormat vulkan specification of the color format
     * @param clearColor clear values in rgba format
     * @param isExternalInput if true the resulting image can be used as input in a later render step
     */
    void addColorAttachment(VkFormat colorFormat, glm::vec4 clearColor = glm::vec4(0.0f), bool isExternalInput = false);

    /**
     * Add a depth attachment to the most recently added subpass.
     * 
     * @param depthFormat vulkan specification of the depth format
     * @param clearDepth clear value for the depth buffer
     * @param isExternalInput if true the resulting image can be used as input in a later render step
     */
    void addDepthAttachment(VkFormat depthFormat, float clearDepth = 1.0f, bool isExternalInput = false);

    /**
     * Add a color attachment to write to the swap chain images.
     * 
     * @param swapChain vulkan handle of the swap chain
     * @param swapChainFormat vulkan specification of the color format used by the swap chain
     * @param clearColor clear values in rgba format
     */
    void addSwapChainAttachment(VkSwapchainKHR swapChain, VkFormat swapChainFormat, glm::vec4 clearColor = glm::vec4(0.0f));

    /**
     * Add an output image from an earlier subpass as input for the most recently added subpass.
     * 
     * @param srcSubPass index of the source subpass providing the input
     * @param srcAttachment index of the attachment in the source subpass
     */
    void addSubPassInput(uint32_t srcSubPass, uint32_t srcAttachment);

    /**
     * Add an external output image from an earlier renderpass as input for the most recently added subpass.
     * 
     * @param imageView vulkan image view of the input image
     * @param isDepth if true the input image stems from a depth attachment
     */
    void addRenderPassInput(VkImageView imageView, bool isDepth);

    /**
     * Initialize subpasses, attachments and underlying images, and framebuffers.
     * 
     * Has to be called before using the render output.
     * Subpasses, attachments, and inputs cannot be changed after.
     */
    void init();

    /**
     * Activate the render output.
     * 
     * Commands recorded after this point render to this output.
     * The first subpass is active until switchSubPass is called.
     * 
     * @param commandBuffer graphics command buffer receiving the begin renderpass command
     * @param frameIndex index of the swap chain image to render to
     */
    void start(VkCommandBuffer commandBuffer, uint32_t frameIndex);

    /**
     * Switch the current subpass.
     * 
     * The dstSubPass has to succeed the current one in the m_subPasses list.
     * Once activated the output images of the new subpass are cleared.
     * 
     * @param commandBuffer graphics command buffer receiving the next subpass command(s)
     * @param dstSubPass index of the subpass to switch to
     */
    void switchSubPass(VkCommandBuffer commandBuffer, uint32_t dstSubPass);

    /**
     * Deactivate the render output.
     * 
     * Commands recorded after this point do not render to this output.
     * 
     * @param commandBuffer graphics command buffer receiving the end renderpass command
     */
    void end(VkCommandBuffer commandBuffer);

    /**
     * Destroy all vulkan components.
     * 
     * Renderpass, framebuffers, and images are destroyed in reverse order of creation.
     */
    void cleanUp();

private:
    /**
     * Initialize the actual output images.
     * 
     * The images themselves have already been created when the attachments were added.
     * Format, aspect, layout, etc. should also be set correctly at this point.
     * Here the actual VkImage, VkDeviceMemory, and VkImageView handles are created and allocated.
     */
    void createAttachments();

    /**
     * Create vulkan renderpass with all subpasses and attachments.
     * 
     * Attachment descriptions and references are derived from m_attachments.
     * Subpass descriptions and dependencies are gathered.
     * Subpass inputs are implemented via attachment references and subpass dependencies.
     */
    void createSubPasses();

    /**
     * Create vulkan framebuffers with the image views of the output images.
     */
    void createFramebuffers();

    std::shared_ptr<Context> m_context; /**< Pointer to the vulkan context */

    uint32_t m_numFramesInFlight; /**< Number of images necessary for swap chain attachment */
    VkExtent2D m_imageExtent; /**< Size of the output images in number of pixels */
    uint32_t m_numLayers; /**< Number of layers of each output image */

    VkViewport m_viewport{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}; /**< Viewport definition to render to a section of the screen */
    VkRect2D m_scissor{0, 0, {0, 0}}; /**< Scissor region to restrict the rendering area */

    VkRenderPass m_renderPass = VK_NULL_HANDLE; /**< Vulkan renderpass encompassing all subpasses and attachments */

    uint32_t m_numSubPasses = 0; /**< Number of subpasses in the vulkan renderpass */
    std::vector<SubPass> m_subPasses; /**< List of separate subpass output sets */

    uint32_t m_numAttachments = 0; /**< Total number of attachments */
    std::vector<Attachment> m_attachments; /**< List of output attachments */

    uint32_t m_numMultisampledImages = 0; /**< Total number of multisampled images */
    uint32_t m_numResolveImages = 0; /**< Total numbers of multisampling resolve images */
    std::vector<Image> m_images; /**< List of output images including multisampling resolve images */

    uint32_t m_numSubPassInputs = 0;

    std::vector<VkFramebuffer> m_frameBuffers; /**< Framebuffers for frames in flight */

    uint32_t m_currentSubPass = 0; /**< Index of the active subpass */

};

#endif //SLBVULKAN_RENDEROUTPUT_H