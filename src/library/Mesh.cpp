#include "Mesh.h"

Mesh::Mesh() {
    //basic triangle for testing
    addVertex(
        glm::vec3(-0.5f, -0.5f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    addVertex(
        glm::vec3(0.5f, -0.5f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    addVertex(
        glm::vec3(0.0f, 0.5f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec2(0.5f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    for(uint16_t i=0; i<3; i++) {
        addIndex(i);
    }
}

bool Mesh::hasBuffers() {
    return m_hasBuffers;
}

void Mesh::addVertex(glm::vec3 position, glm::vec3 normal, glm::vec2 texCoord, glm::vec3 tangent) {
    m_vertices.emplace_back(Vertex{
        glm::vec4(position, 1.0f),
        glm::normalize(normal),
        texCoord,
        glm::normalize(tangent)
    });
}

void Mesh::addIndex(uint16_t index) {
    m_indices.emplace_back(index);
}

void Mesh::createBuffers(std::shared_ptr<Context> &context) {
    if(m_hasBuffers) {
        throw std::runtime_error("MESH ERROR: Buffers have already been created.");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    void* data;

    //fill staging buffer with vertex data
    auto vertexSize = static_cast<VkDeviceSize>(m_vertices.size() * sizeof(Vertex));
    context->createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    vkMapMemory(context->getDevice(), stagingBufferMemory, 0, vertexSize, 0, &data);
    memcpy(data, m_vertices.data(), (size_t) vertexSize);
    vkUnmapMemory(context->getDevice(), stagingBufferMemory);

    //transfer staging buffer to vertex buffer
    context->createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexMemory);
    context->copyBuffer(stagingBuffer, m_vertexBuffer, vertexSize);
    vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(context->getDevice(), stagingBufferMemory, nullptr);

    //fill staging buffer with index data
    auto indexSize = static_cast<VkDeviceSize>(m_indices.size() * sizeof(uint16_t));
    context->createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    vkMapMemory(context->getDevice(), stagingBufferMemory, 0, indexSize, 0, &data);
    memcpy(data, m_indices.data(), (size_t) indexSize);
    vkUnmapMemory(context->getDevice(), stagingBufferMemory);

    //transfer staging buffer to index buffer
    context->createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexMemory);
    context->copyBuffer(stagingBuffer, m_indexBuffer, indexSize);
    vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(context->getDevice(), stagingBufferMemory, nullptr);

    m_hasBuffers = true;
}

void Mesh::render(VkCommandBuffer commandBuffer, uint32_t numInstances) {
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, m_indices.size(), numInstances, 0, 0, 0);
}

void Mesh::cleanUp(std::shared_ptr<Context> &context) {
    vkDestroyBuffer(context->getDevice(), m_vertexBuffer, nullptr);
    vkFreeMemory(context->getDevice(), m_vertexMemory, nullptr);
    vkDestroyBuffer(context->getDevice(), m_indexBuffer, nullptr);
    vkFreeMemory(context->getDevice(), m_indexMemory, nullptr);

    m_hasBuffers = false;
}