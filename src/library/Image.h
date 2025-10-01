#ifndef VKLEO_IMAGE_H
#define VKLEO_IMAGE_H

#include <glm/glm.hpp>

#include "Context.h"

/**
 * Image or set of images used to access in shaders or to render to.
 * 
 * Can be multisampled, can have multiple layers, can be loaded from an image file.
 * Manages vulkan handle, memory, and image view.
 * For certain cases, like the swap chain, multiple frames in flight are combined in a single image object.
 */
class Image {
public:
    /**
     * Create an empty image for a specified purpose.
     * 
     * The image is not initialized until createAndAllocate, useSwapChain, or loadTexture are called.
     * Typical VkImageUsageFlags include VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, etc.
     * 
     * @param width width of the image in number of pixels
     * @param height height of the image in number of pixels
     * @param usage vulkan flags indicated the purpose of the image
     */
    Image(uint32_t width, uint32_t height, VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT);

    Image(std::shared_ptr<Context> &context, const std::string &fileName);

    ~Image();

    /**
     * Return the format of the image values.
     */
    VkFormat getFormat();

    /**
     * Return the main layout of the image.
     */
    VkImageLayout getLayout();

    /**
     * Return the initial layout at the beginning of a renderpass.
     * 
     * This is relevant for output images where the layout is automatically changed during a renderpass.
     */
    VkImageLayout getInitialLayout();

    /**
     * Return the final layout at the end of a renderpass.
     * 
     * This is relevant for output images where the layout is automatically changed during a renderpass.
     */
    VkImageLayout getFinalLayout();

    /**
     * Return the vulkan image view as an interface to access the image data.
     * 
     * Necessary for shader access to the image.
     * 
     * @param frame (optional) index of the current frame in flight
     */
    VkImageView getView(uint32_t frame = 0);

    /**
     * Return the availability of the image data.
     * 
     * @return true if the vulkan handle, memory, and image view have been initialized
     */
    bool hasHandle();

    /**
     * Return multisampling status.
     * 
     * If multisampling is used multiple samples are stored for each pixel.
     * 
     * @return true if multisampling is used when rendering to and accessing the image
     */
    bool usesMultisampling();

    /**
     * Change to number of layers the image consists of.
     * 
     * Per default the number of layers is 1, otherwise the image type (in the image view) is VK_IMAGE_VIEW_TYPE_2D_ARRAY.
     * 
     * @param numLayers new number of layers
     */
    void setNumLayers(uint32_t numLayers);

    /**
     * Change the format of the image values.
     * 
     * @param format vulkan specification of the new image format
     */
    void setFormat(VkFormat format);

    /**
     * Change the aspect flags included in the image view.
     * 
     * The aspect flags dictate which parts of the image data the image view refers to.
     * Typical VkImageAspectFlags include VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, etc.
     * 
     * @param aspect new aspect flags encoded as a bitmask
     */
    void setAspect(VkImageAspectFlags aspect);

    /**
     * Change the main layout of the image.
     * 
     * @param layout vulkan specification of the new image layout
     */
    void setLayout(VkImageLayout layout);

    /**
     * Change the initial layout of the image at the beginning of a renderpass.
     * 
     * This is relevant for output images where the layout is automatically changed during a renderpass.
     * 
     * @param initial vulkan specification of the new image layout
     */
    void setInitialLayout(VkImageLayout initial);

    /**
     * Change the final layout of the image at the end of a renderpass.
     * 
     * This is relevant for output images where the layout is automatically changed during a renderpass.
     * 
     * @param final vulkan specification of the new image layout
     */
    void setFinalLayout(VkImageLayout final);

    /**
     * Add another usage so the image can be used for an additional purpose.
     * 
     * The values is combined with the other uses in the vulkan usage bitmask.
     * 
     * @param usage vulkan usage flags encoded as a bitmask
     */
    void addUsage(VkImageUsageFlags usage);

    /**
     * Turn the image into a multisampled one.
     * 
     * If enabled multiple samples are stored for each pixel.
     */
    void enableMultisampling();

    /**
     * Create vulkan representation of the image.
     * 
     * Initializes vulkan handles, allocates and binds image memory.
     * A pointer to the vulkan context is used to access the logical device.
     * 
     * @param context pointer to the vulkan context
     * @param numFrames number of images created for multiple frames in flight
     */
    void createAndAllocate(std::shared_ptr<Context> &context, uint32_t numFrames = 1);

    /**
     * Extract and store images from a swap chain.
     * 
     * A pointer to the vulkan context is used to access the logical device.
     * 
     * @param context pointer to the vulkan context
     * @param swapChain vulkan handle of the swap chain
     */
    void useSwapChain(std::shared_ptr<Context> &context, VkSwapchainKHR swapChain);

    /**
     * Create image views as an interface to access the image data.
     * 
     * Necessary for shader access to the image.
     * If multiple images were created for frames in flight a separate image view is created for each of them.
     * A pointer to the vulkan context is used to access the logical device.
     * 
     * @param context pointer to the vulkan context
     */
    void createViews(std::shared_ptr<Context> &context);

    /**
     * Execute a change in image layout.
     * 
     * A pointer to the vulkan context is used to execute the one-off GPU command.
     * 
     * @param context pointer to the vulkan context
     * @param oldLayout layout of the image before the transition
     * @param newLayout layout of the image after the transition
     */
    void transitionLayout(std::shared_ptr<Context> &context, VkImageLayout oldLayout, VkImageLayout newLayout);

    /**
     * Copy the contents of a buffer into the image.
     * 
     * A pointer to the vulkan context is used to execute the one-off GPU command.
     * 
     * @param context pointer to the vulkan context
     * @param buffer source buffer to copy from
     */
    void copyBuffer(std::shared_ptr<Context> &context, VkBuffer buffer);

    /**
     * Load image data from a file.
     * 
     * A pointer to the vulkan context is used to access the logical device.
     * 
     * @param context pointer to the vulkan context
     * @param fileName name of an image file in the resources/textures folder
     */
    void loadTexture(std::shared_ptr<Context> &context, const std::string &fileName);

    /**
     * Destroy all vulkan components.
     * 
     * All images and image views are destroyed, image memory is freed up.
     * A pointer to the vulkan context is used to access the logical device.
     * 
     * @param context pointer to the vulkan context
     */
    void cleanUp(std::shared_ptr<Context> &context);

private:
    uint32_t m_width; /**< Width of the image in number of pixels */
    uint32_t m_height; /**< Height of the image in number of pixels */
    uint32_t m_numLayers = 1; /**< Number of layers relevant for an image array */

    VkFormat m_format = VK_FORMAT_R8G8B8A8_SRGB; /**< Format of the image values */
    VkImageAspectFlags m_aspect = VK_IMAGE_ASPECT_COLOR_BIT; /**< Image aspect flags included in the image view */
    VkImageUsageFlags m_usage; /**< Usage flags indicating for what purposes the image will be used */
    VkMemoryPropertyFlags m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; /**< Properties the allocated image memory has to fulfil */

    VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED; /**< Main image layout */
    VkImageLayout m_initial = VK_IMAGE_LAYOUT_UNDEFINED; /**< Initial image layout at the beginning of a renderpass (relevant for ouput images) */
    VkImageLayout m_final = VK_IMAGE_LAYOUT_UNDEFINED; /**< Final image layout at the end of a renderpass (relevant for output images) */

    bool m_useMultisampling = false; /**< If true multiple samples are stored for each pixel */

    std::vector<VkImage> m_handles; /**< Vulkan handles of the created images */
    std::vector<VkDeviceMemory> m_memory; /**< Memory containing the image data */
    std::vector<VkImageView> m_views; /**< Image views necessary for shader access to the image */

};

#endif //VKLEO_IMAGE_H