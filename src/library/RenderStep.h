#ifndef SLBVULKAN_RENDERSTEP_H
#define SLBVULKAN_RENDERSTEP_H

#include "Context.h"
#include "ResourceLoader.h"
#include "DescriptorSet.h"
#include "RenderOutput.h"
#include "Mesh.h"

/**
 * Render mode dictating the type of compute or draw call executed in a render step.
 */
enum RenderMode {
    renderMeshes, /**< Instanced render call for each mesh in the scene */
    renderLightProxies, /**< Deferred rendering of proxy geometry for each light source in the scene */
};

/**
 * Individual step in the rendering process.
 * 
 * Manages a vulkan pipeline, a shader set, and different render settings.
 */
class RenderStep {
public:
    /**
     * Create an unspecified render step.
     * 
     * A new smart pointer to the vulkan context is stored for later use.
     * The render step cannot be used until createShaderModules, and initRenderStep have been called.
     * 
     * @param context pointer to the vulkan context
     * @param numFramesInFlight number of images alternated in the swap chain
     */
    RenderStep(std::shared_ptr<Context> &context, uint32_t numFramesInFlight);
    ~RenderStep();

    /**
     * Return the pipeline layout e.g. for push constant commands.
     * 
     * @return vulkan handle of the pipeline layout
     */
    VkPipelineLayout getPipelineLayout();

    /**
     * Return the type of compute or draw call executed in the render step.
     */
    RenderMode getRenderMode();

    /**
     * Return the dispatch size or number of instances of the compute or draw call.
     */
    uint32_t getRenderSize();

    /**
     * Return the index of the render output this step renders to.
     */
    uint32_t getOutputIndex();

    /**
     * Return the index of the subset of images within the render output.
     */
    uint32_t getSubPassIndex();

    /**
     * Change the name displayed as debug label.
     * 
     * @param name new name describing the render step
     */
    void setName(std::string name);

    /**
     * Change the type of compute or draw call executed by this render step.
     */
    void setRenderMode(RenderMode mode, uint32_t renderSize = 1);

    /**
     * Load shader files and create shader modules.
     * 
     * The file names should end in ".vert", ".geom", ".frag", or ".comp" to denote different shader stages.
     * Shader contents are compiled into spir-v format and then loaded.
     * Required descriptor sets denoted with "#include ..." in the shader files are collected and added to m_descriptorSets.
     * 
     * @param shaderFiles names of shader files in the resources/shaders/file
     * @param descriptorSets list of all shader resource sets that the required subset is extracted from
     * @param sceneCounts numbers of different components in the scene
     */
    void createShaderModules(const std::vector<std::string> &shaderFiles, std::vector<DescriptorSet> &descriptorSets, std::vector<uint32_t> &sceneCounts);

    /**
     * Change the culling settings for rendering.
     * 
     * Per default culling is set to VK_CULL_MODE_NONE.
     * If it is set to VK_CULL_MODE_BACK_BIT only front faces are rendered.
     * If it is set to VK_CULL_MODE_FRONT_BIT only back faces are rendered.
     * 
     * @param mode vulkan specification of the new cull mode
     */
    void setCullMode(VkCullModeFlags mode);

    /**
     * Activate blending during this render step.
     * 
     * Blend factors are all set to one.
     */
    void enableBlending();

    /**
     * Set up vulkan pipeline with the specified shaders and render settings.
     * 
     * Shaders have to be loaded before this point and cannot be changed after.
     * 
     * @param output set of output images this step renders to
     * @param subPassIndex index of the subpass the step renders to
     */
    void initRenderStep(RenderOutput &output, uint32_t subPassIndex);

    /**
     * Activate render step.
     * 
     * Commands recorded after this point use this pipeline.
     * 
     * @param commandBuffer graphics command buffer receiving the bind pipeline command
     * @param frameIndex index of the swap chain image to render to
     */
    void start(VkCommandBuffer commandBuffer, uint32_t frameIndex);

    /**
     * Deactivate render step.
     * 
     * Commands recorded after this point do not use this pipeline.
     * 
     * @param commandBuffer graphics command buffer receiving the end debug label command
     */
    void end(VkCommandBuffer commandBuffer);

    /**
     * Destroy all vulkan components.
     * 
     * Vulkan pipeline, pipeline layout, and shader modules are destroyed in reverse order of creation.
     */
    void cleanUp();

private:
    /**
     * Return the shader stage matching the suffix of a file name.
     * 
     * @param fileName name of a shader file
     * @return vulkan shader stage flag
     */
    VkShaderStageFlagBits getShaderStage(const std::string &fileName);

    std::shared_ptr<Context> m_context; /**< Pointer to the vulkan context */
    uint32_t m_numFramesInFlight; /**< Number of images alternated in the swap chain */

    std::string m_name = "Unnamed Render Step"; /**< Name describing the render step */
    VkPipelineBindPoint m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; /**< Pipeline bind point distinguishing compute from graphics */

    std::vector<VkShaderModule> m_shaderModules; /**< Vulkan handles of the shader modules */
    std::vector<VkShaderStageFlagBits> m_shaderStages; /**< Vulkan shader stage flags for each shader module */

    std::vector<uint32_t> m_requiredDescriptorSets; /**< Absolute indices of the required descriptor sets */
    std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts; /**< Layouts of the required descriptor sets */
    std::vector<std::vector<VkDescriptorSet>> m_descriptorSets; /**< Required descriptor sets for each frame in flight */

    VkPrimitiveTopology m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; /**< Topology dictating how primitives are assembled for the rendered geometry */
    VkCullModeFlags m_cullMode = VK_CULL_MODE_NONE; /**< Culling settings */
    bool m_useDepth = true; /**< If true depth testing is enabled in the vulkan pipeline */
    bool m_useBlending = false; /**< If true blending is enabled in the vulkan pipeline */

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE; /**< Vulkan pipeline layout encompassing descriptor sets and push constants */
    VkPipeline m_pipeline = VK_NULL_HANDLE; /**< Vulkan pipeline containing all relevant settings and components of the render step */

    RenderMode m_renderMode = renderMeshes; /**< Main compute or draw command */
    uint32_t m_renderSize = 1; /**< Dispatch size or number of instances of the main compute or draw command */
    uint32_t m_outputIndex = 0; /**< Index of the output this step renders to within the renderer */
    uint32_t m_subPassIndex = 0; /**< Index of the subpass within the render output */

};

#endif //SLBVULKAN_RENDERSTEP_H