#ifndef SLBVULKAN_MESH_H
#define SLBVULKAN_MESH_H

#include <glm/ext.hpp>

#include "Context.h"

/**
 * Vertex definition describing attributes and their bindings in the shader.
 */
struct Vertex {
    glm::vec4 position; /**< Position in local, homogeneous coordinates */
    glm::vec3 normal; /**< Normal in local coordinates */
    glm::vec2 texCoord; /**< Texture coordinates in uv coordinates */
    glm::vec3 tangent; /**< Tangent in local coordinates */

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    };
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, tangent);
        return attributeDescriptions;
    };
};

/**
 * Geometry composed of triangles defined on a set of vertices.
 * Rendering is indexed by default.
 */
class Mesh {
public:
    /**
     * Create an empty mesh.
     * 
     * Mesh is not initialized until createBuffers is called.
     */
    Mesh();
    ~Mesh() = default;

    /**
     * Return the availability of the mesh for rendering.
     * 
     * @return true if the buffers have been created and initialized
     */
    bool hasBuffers();

    /**
     * Add a vertex in local coordinates to the vertex list.
     * 
     * The vertex is not be rendered by default, it has to be integrated via addIndex.
     * Attributes normal, texCoord, and tangent are interpolated over the triangles.
     * Normal and tangent are normalized before being added to the list (just to make sure).
     * 
     * @param position position of the vertex in local coordinates
     * @param normal normal of the vertex in local coordinates
     * @param texCoord texture coordinates of the vertex
     * @param tangent tangent of the vertex in local coordinates
     */
    void addVertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texCoord, glm::vec3 tangent);

    /**
     * Add an index to integrate a vertex into the triangle topology.
     * 
     * @param index index of a vertex in the vertex buffer
     */
    void addIndex(uint16_t index);

    /**
     * Add a sphere to the geometry.
     * 
     * @param center position of the sphere center
     * @param radius radius of the sphere in local coordinates
     * @param resolution number of segments the sphere is divided in horizontally
     */
    void addSphere(glm::vec3 center, float radius, int resolution);

    /**
     * Add a cone to the geometry.
     * 
     * @param base position of the cone base
     * @param radius radius of the cone base
     * @param height distance from base to tip
     * @param int resolution number of segments the cone is divided in horizontally
     */
    void addCone(glm::vec3 base, float radius, float height, int resolution);

    /**
     * Reassign tangents for all added vertices.
     * 
     * Tangents are generated for the triangles defined via the index list.
     * Positions and texture coordinates of the vertices are used to calculate the tangents.
     */
    void calculateTangents();

    /**
     * Create vulkan representation of the mesh.
     * 
     * Has to be called before rendering the mesh.
     * The geometry cannot be changed after.
     * Vertex and index buffers are created, memory is allocated and filled with the specified data.
     * A pointer to the vulkan context is used to access the logical device.
     * 
     * @param context pointer to the vulkan context
     */
    void createBuffers(std::shared_ptr<Context> &context);

    /**
     * Add draw command to a provided command buffer.
     * 
     * Instanced rendering is used if numInstances > 1.
     * 
     * @param commandBuffer graphics command buffer
     * @param numInstances number of instances of the mesh
     */
    void render(VkCommandBuffer commandBuffer, uint32_t numInstances);

    /**
     * Destroy all vulkan components.
     * 
     * Vertex and index buffers are destroyed and the associated memory is freed up.
     * A pointer to the vulkan context is used to access the logical device.
     * 
     * @param context pointer to the vulkan context
     */
    void cleanUp(std::shared_ptr<Context> &context);
    
private:
    /**
     * Calculate the tangent for a vertex from position and texture coordinates.
     * 
     * The tangent is determined for the vertex with index i0.
     * The other vertices making up the triangle also have to be specified as index parameters.
     * 
     * @param i0 index of the vertex the tangent is assigned to
     * @param i1 index of the next vertex in the triangle
     * @param i2 index of the previous vertex in the triangle
     * @return normalized tangent of the vertex with index i0
     */
    glm::vec3 getTangent(uint16_t i0, uint16_t i1, uint16_t i2);

    std::vector<Vertex> m_vertices; /**< List of vertices with required attributes */
    std::vector<uint16_t> m_indices; /**< List of indices assembling the vertices into triangles */

    bool m_hasBuffers = false; /**< Status of the buffers required for rendering */

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE; /**< Vulkan handle of the vertex buffer */
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE; /**< Memory containing the vertex data */

    VkBuffer m_indexBuffer = VK_NULL_HANDLE; /**< Vulkan handle of the index buffer */
    VkDeviceMemory m_indexMemory = VK_NULL_HANDLE; /**< Memory containing the index data */

};

#endif //SLBVULKAN_MESH_H