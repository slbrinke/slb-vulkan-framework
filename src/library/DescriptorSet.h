#ifndef SLBVULKAN_DESCRIPTORSET_H
#define SLBVULKAN_DESCRIPTORSET_H

#include <glm/glm.hpp>

#include "Context.h"

/**
 * Vulkan representation of an individual shader resource.
 * 
 * The resource can either be a buffer or an image.
 * If it is a buffer numImages is 0 and type is either VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or VK_DESCRIPTOR_TYPE_STORAGE_BUFFER.
 * If it is an image bufferSize is 0 and type is either VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT.
 * A resource can have more than one binding in the descriptor set layout, e.g. to access data associated with the preceding frame in flight.
 */
struct Descriptor {
    std::string name; /**< Unique name identifying the resource on the C++ side */

    uint32_t firstBinding; /**< First index in the total bindings list that belongs to this resource */
    uint32_t numBindings; /**< Number of layout bindings this resource takes up in the descriptor set layout */
    VkDescriptorType type; /**< Vulkan specification of the descriptor type */
    
    VkDeviceSize bufferSize = 0; /**< Size of the uniform or storage buffer */
    std::vector<VkBuffer> buffers; /**< Vulkan handles of the buffers for each frame in flight */
    std::vector<VkDeviceMemory> memory; /**< Memory containing the buffer data */
    std::vector<void*> buffersMapped; /**< Pointers the buffers are mapped to (persistent mapping) */

    uint32_t numImages = 0; /**<  */
    std::vector<VkImageView> imageViews; /**< Image views the descriptor points to */
    
};

/**
 * Set of buffer and image resources that can be accessed on the GPU.
 * 
 * Manages vulkan descriptor sets and the associated layout.
 */
class DescriptorSet {
public:
    /**
     * Create an empty descriptor set.
     * 
     * A new smart pointer to the vulkan context is stored for later use.
     * Layout and descriptor sets cannot be accessed until init has been called.
     * 
     * @param context pointer to the vulkan context
     * @param numFramesInFlight number of images alternated in the swap chain
     */
    DescriptorSet(std::shared_ptr<Context> &context, uint32_t numFramesInFlight);
    ~DescriptorSet();

    /**
     * Return the descriptor set layout specifying the resource bindings.
     */
    VkDescriptorSetLayout getLayout();

    /**
     * Return the descriptor set for a given frame in flight.
     * 
     * @param frameIndex index of the current swap chain image
     */
    VkDescriptorSet getSet(uint32_t frameIndex);

    /**
     * Add a buffer resource to the descriptor set.
     * 
     * A new buffer with the specified size is created for each frame in flight.
     * If data is provided it is copied into each of the buffers.
     * 
     * If doubleBinding is true the descriptor is initialized with two bindings in the descriptor set layout.
     * And in the descriptor set itself the first binding refers to the buffer associated with the previous frame in flight.
     * 
     * @param name unique name identifying the resource for later access
     * @param descriptorType type distinguishing between VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER and VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
     * @param bufferSize size of the new buffer
     * @param doubleBinding if true an additional binding is added for the previous frame
     * @param data initial buffer data
     */
    void addBuffer(std::string name, VkDescriptorType descriptorType, VkDeviceSize bufferSize, bool doubleBinding = false, const void *data = nullptr);

    /**
     * Add an image resource to the descriptor set.
     * 
     * If the descriptorType is VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER and m_imageSampler is VK_NULL_HANDLE, the sampler is created here.
     * 
     * @param descriptorType type distinguishing between VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER and VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
     * @param imageView image view as an interface to access image data
     */
    void addImage(VkDescriptorType descriptorType, VkImageView imageView);

    /**
     * Add an image resource to the descriptor set.
     * 
     * If the descriptorType is VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER and m_imageSampler is VK_NULL_HANDLE, the sampler is created here.
     * 
     * @param descriptorType type distinguishing between VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER and VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
     * @param imageView image views as interfaces to access image data
     */
    void addImages(VkDescriptorType descriptorType, std::vector<VkImageView> &imageView);

    /**
     * Initializes descriptor set layout, descriptor pool, and the descriptor sets themselves.
     * 
     * This has to be called before the resources can be accessed in a shader.
     */
    void init();

    /**
     * Modify the data in one of the buffers.
     * 
     * @param name unique name identifying the resource
     * @param frameIndex index of the current frame in flight
     * @param data new data copied into the buffer
     */
    void updateBuffer(std::string name, uint32_t frameIndex, void* data);

    /**
     * Set buffer contents to zero.
     * 
     * @param name unique name identifying the resource
     * @param commandBuffer command buffer receiving the fill buffer command
     * @param frameIndex index of the current frame in flight
     */
    void clearBuffer(std::string name, VkCommandBuffer commandBuffer, uint32_t frameIndex);

    /**
     * Copy buffer contents between frames.
     * 
     * Data is copied from the buffer belonging to frame (frameIndex-1)%m_numFramesInFlight.
     * Data is copied to the buffer belonging to frame frameIndex.
     * 
     * @param name unique name identifying the resource
     * @param frameIndex index of the current frame in flight
     */
    void copyBufferFromLastFrame(std::string name, uint32_t frameIndex);

    /**
     * Destroy all vulkan components.
     * 
     * Descriptor pool, descriptor set layout, and buffers are destroyed.
     * Buffer memory is freed up.
     * If an image sampler was created at some point, it is also destroyed here.
     */
    void cleanUp();

private:
    /**
     * Create descriptor set layout and descriptor pool.
     * 
     * Bindings are iterated to gather layout bindings and pool sizes.
     */
    void createLayoutAndPool();

    /**
     * Create descriptor sets for each frame in flight.
     * 
     * Bindings are iterated again to gather resource data for the respective frames.
     */
    void createSets();

    std::shared_ptr<Context> m_context; /**< Pointer to the vulkan context */
    uint32_t m_numFramesInFlight; /**< Number of images alternated in the swap chain */

    uint32_t m_numDescriptors = 0; /**< Total number of resources included in the set */
    std::vector<Descriptor> m_descriptors; /**< Buffer and image resources */
    uint32_t m_numBufferBindings = 0; /**< Total number of buffer bindings in the descriptor set layout */
    uint32_t m_numImageBindings = 0; /**< Total number of image bindings in the descriptor set layout */
    uint32_t m_numImages = 0; /**< Total number of image views added to the descriptor set */

    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE; /**< Vulkan handle of the descriptor set layout */
    VkDescriptorPool m_pool = VK_NULL_HANDLE; /**< Vulkan handle of the descriptor pool the sets are allocated from */
    std::vector<VkDescriptorSet> m_sets; /**< Vulkan handles of the descriptor sets for each frame in flight */

    VkSampler m_imageSampler = VK_NULL_HANDLE; /**< Optional image sampler required for VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER descriptors */
};

#endif //SLBVULKAN_DESCRIPTORSET_H