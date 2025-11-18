#ifndef SLBVULKAN_STEP_H
#define SLBVULKAN_STEP_H

#include "Context.h"
#include "ResourceLoader.h"
#include "DescriptorSet.h"
#include "RenderOutput.h"
#include "Mesh.h"

/**
 * Individual step within a rendering or compute process.
 * 
 * Encapsulates a vulkan pipeline with a set of shaders.
 * Manages the descriptor sets required by the shaders.
 * Can be added to a list of steps in a Renderer.
 * Stores what type of command is recorded by the Renderer.
 */
class Step {
public:
    Step(std::shared_ptr<Context> &context, uint32_t numFramesInFlight);
    ~Step();

    /**
     * Return the pipeline layout e.g. for push constant commands.
     * 
     * @return vulkan handle of the pipeline layout
     */
    VkPipelineLayout getPipelineLayout();

    /**
     * Change the name displayed as debug label.
     * 
     * @param name new name describing the render or compute step
     */
    void setName(std::string name);

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
    virtual void createShaderModules(const std::vector<std::string> &shaderFiles, std::vector<DescriptorSet> &descriptorSets, std::vector<uint32_t> &sceneCounts);

    /**
     * Activate render or compute step.
     * 
     * Commands recorded after this point use this pipeline.
     * 
     * @param commandBuffer command buffer receiving the bind pipeline command
     * @param frameIndex index of the current swap chain image
     */
    void start(VkCommandBuffer commandBuffer, uint32_t frameIndex);

    /**
     * Deactivate render or compute step.
     * 
     * Commands recorded after this point do not use this pipeline.
     * 
     * @param commandBuffer command buffer receiving the end debug label command
     */
    void end(VkCommandBuffer commandBuffer);

    /**
     * Destroy all vulkan components.
     * 
     * Vulkan pipeline, pipeline layout, and shader modules are destroyed in reverse order of creation.
     */
    void cleanUp();

protected:
    /**
     * Return the shader stage matching the suffix of a file name.
     * 
     * @param fileName name of a shader file
     * @return vulkan shader stage flag
     */
    VkShaderStageFlagBits getShaderStage(const std::string &fileName);

    std::shared_ptr<Context> m_context; /**< Pointer to the vulkan context */
    uint32_t m_numFramesInFlight; /**< Number of images alternated in the swap chain */

    std::string m_name = "Unnamed Render/Compute Step"; /**< Name describing the render/compute step */
    VkPipelineBindPoint m_bindPoint; /**< Pipeline bind point distinguishing compute from graphics */

    std::vector<VkShaderModule> m_shaderModules; /**< Vulkan handles of the shader modules */
    std::vector<VkShaderStageFlagBits> m_shaderStages; /**< Vulkan shader stage flags for each shader module */

    std::vector<uint32_t> m_requiredDescriptorSets; /**< Absolute indices of the required descriptor sets */
    std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts; /**< Layouts of the required descriptor sets */
    std::vector<std::vector<VkDescriptorSet>> m_descriptorSets; /**< Required descriptor sets for each frame in flight */

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE; /**< Vulkan pipeline layout encompassing descriptor sets and push constants */
    VkPipeline m_pipeline = VK_NULL_HANDLE; /**< Vulkan pipeline containing all relevant settings and components of the render step */

};

/**
 * Render mode dictating the type of draw call executed in a render step.
 */
enum RenderMode {
    renderMeshes, /**< Instanced render call for each mesh in the scene */
    renderLightProxies, /**< Deferred rendering of proxy geometry for each light source in the scene */
    renderEnvMap, /**< Environment map displayed on a sphere around the camera */
    renderInstancedPoint /**< Simple instanced render call with a dummy mesh containing a single vertex */
};

/**
 * Individual step within a rendering process.
 * 
 * Has to be connected to a specific RenderOutput to function.
 * Manages render settings like primitive assembly, culling, and blending.
 */
class RenderStep : public Step {
public:
    RenderStep(std::shared_ptr<Context> &context, uint32_t numFramesInFlight);
    ~RenderStep() = default;

    /**
     * Return the type of draw call executed in the render step.
     */
    RenderMode getRenderMode();

    /**
     * Return the number of instances of the draw call.
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
     * Change the type of draw call executed by this render step.
     * 
     * @param mode type of draw call
     * @param size number of instances rendered of each mesh
     */
    void setRenderMode(RenderMode mode, uint32_t size = 1);

    /**
     * Change the type of primitives rendered by this render step.
     * 
     * Typical values include VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, etc.
     */
    void setPrimitiveTopology(VkPrimitiveTopology topology);

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

private:

    VkPrimitiveTopology m_primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; /**< Topology dictating how primitives are assembled for the rendered geometry */
    VkCullModeFlags m_cullMode = VK_CULL_MODE_NONE; /**< Culling settings */
    bool m_useDepth = true; /**< If true depth testing is enabled in the vulkan pipeline */
    bool m_useBlending = false; /**< If true blending is enabled in the vulkan pipeline */

    RenderMode m_renderMode = renderMeshes; /**< Type of draw call executed as the main command */
    uint32_t m_renderSize = 1; /**< Number of instances rendered of each mesh */

    uint32_t m_outputIndex = 0; /**< Index of the output this step renders to within the renderer */
    uint32_t m_subPassIndex = 0; /**< Index of the subpass within the render output */
    
};

/**
 * Compute mode dictating the type of dispatch executed in a compute step.
 */
enum ComputeMode {
    computeSimple, /**< Simple compute command with a fixed size */
    computeCascaded /**< Sequence of compute commands with varying work group size */
};

class ComputeStep : public Step {
public:
    ComputeStep(std::shared_ptr<Context> &context, uint32_t numFramesInFlight);
    ~ComputeStep() = default;

    /**
     * Return the compute mode.
     * 
     * @return type of compute command
     */
    ComputeMode getComputeMode();

    /**
     * Return the size of the compute dispatch.
     * 
     * @return number of invocations of the compute command
     */
    uint32_t getComputeSize();

    /**
     * Change the type of command executed by this compute step.
     * 
     * @param mode type of compute command
     * @param size number of invocations
     */
    void setComputeMode(ComputeMode mode, uint32_t size);

    void createShaderModules(const std::vector<std::string> &shaderFiles, std::vector<DescriptorSet> &descriptorSets, std::vector<uint32_t> &sceneCounts) override;

    /**
     * Set up vulkan pipeline with the specified compute shader.
     */
    void initComputeStep();

private:

    ComputeMode m_computeMode = computeSimple; /**< Type of compute dispatch executed as the main command */
    uint32_t m_computeSize = 100; /**< Number of invocations of the compute shader */
    
};

#endif //SLBVULKAN_STEP_H