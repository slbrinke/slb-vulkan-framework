#include "Mesh.h"

Mesh::Mesh() {
    
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

void Mesh::addIndex(uint32_t index) {
    m_indices.emplace_back(index);
}

void Mesh::addSphere(glm::vec3 center, float radius, int resolution) {
    int halfRes = (resolution + 1) / 2;

    //bottom vertex
    for(int h=0; h<resolution-1; h++) {
        float hRel = (static_cast<float>(h) + 0.5f) / static_cast<float>(resolution-1);
        float phi = 2.0f * glm::pi<float>() * hRel;

        addVertex(
            center + glm::vec3(0.0f, -radius, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec2(hRel, 0.0f),
            glm::vec3(-glm::sin(phi), 0.0f, glm::cos(phi))
        );
    }
    //main vertices
    for(int v=1; v<halfRes; v++) {
        float vRel = static_cast<float>(v) / static_cast<float>(halfRes);
        float theta = glm::pi<float>() * (vRel - 0.5f);
        for(int h=0; h<resolution; h++) {
            float hRel = static_cast<float>(h) / static_cast<float>(resolution-1);
            float phi = 2.0f * glm::pi<float>() * hRel;

            glm::vec3 spherePos = glm::vec3(
                glm::sin(phi) * glm::cos(theta),
                glm::sin(theta),
                glm::cos(phi) * glm::cos(theta)
            );
            addVertex(
                center + radius * spherePos,
                spherePos,
                glm::vec2(hRel, vRel),
                glm::vec3(-glm::sin(phi), 0.0f, glm::cos(phi))
            );
        }
    }
    //top vertex
    for(int h=0; h<resolution-1; h++) {
        float hRel = (static_cast<float>(h) + 0.5f) / static_cast<float>(resolution-1);
        float phi = 2.0f * glm::pi<float>() * hRel;

        addVertex(
            center + glm::vec3(0.0f, radius, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec2(hRel, 1.0f),
            glm::vec3(-glm::sin(phi), 0.0f, glm::cos(phi))
        );
    }

    //bottom faces
    int offset = resolution-1;
    for(int h=0; h<resolution-1; h++) {
        addIndex(offset + h);
        addIndex(h);
        addIndex(offset + h+1);
    }
    //main faces
    for(int v=0; v<halfRes-2; v++) {
        for(int h=0; h<resolution-1; h++) {
            addIndex(offset + v*resolution + h);
            addIndex(offset + v*resolution + h+1);
            addIndex(offset + (v+1)*resolution + h+1);
            addIndex(offset + (v+1)*resolution + h+1);
            addIndex(offset + (v+1)*resolution + h);
            addIndex(offset + v*resolution + h);
        }
    }
    //top faces
    offset += (halfRes-2) * resolution;
    for(int h=0; h<resolution-1; h++) {
        addIndex(offset + h);
        addIndex(offset + h+1);
        addIndex(offset + resolution + h);
    }
}

void Mesh::addCone(glm::vec3 base, float radius, float height, int resolution) {
    glm::vec3 topPos = glm::vec3(0.0f, height, 0.0f);

    for(int h=0; h<resolution; h++) {
        float hStart = static_cast<float>(h) / static_cast<float>(resolution);
        float hMid = (static_cast<float>(h) + 0.5f) / static_cast<float>(resolution);
        float phiStart = 2.0f * glm::pi<float>() * hStart;
        float phiMid = 2.0f * glm::pi<float>() * hMid;

        glm::vec3 startPos = glm::vec3(
            glm::sin(phiStart),
            0.0f,
            glm::cos(phiStart)
        );
        glm::vec3 midPos = glm::vec3(
            glm::sin(phiMid),
            0.0f,
            glm::cos(phiMid)
        );
        glm::vec3 startTangent = glm::normalize(glm::cross(topPos - startPos, startPos));
        glm::vec3 midTangent = glm::normalize(glm::cross(topPos - midPos, midPos));

        addVertex(
            base + topPos,
            glm::cross(midTangent, topPos - midPos),
            glm::vec2(hMid, 1.0f),
            midTangent
        );
        addVertex(
            base + radius * startPos,
            glm::cross(startTangent, topPos - startPos),
            glm::vec2(hStart, 0.5f),
            startTangent
        );
        addVertex(
            base + radius * startPos,
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec2(hStart, 0.5f),
            -startTangent
        );
        addVertex(
            base,
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec2(hMid, 0.0f),
            -midTangent
        );

        addIndex(4 * h + 1);
        addIndex(4 * ((h + 1) % resolution) + 1);
        addIndex(4 * h);
        addIndex(4 * h + 2);
        addIndex(4 * h + 3);
        addIndex(4 * ((h + 1) % resolution) + 2);
    }
}

void Mesh::calculateTangents() {
    for(size_t i=0; i<m_indices.size()/3; i++) {
        std::array<uint32_t,3> triIndices = {
            m_indices[3*i],
            m_indices[3*i+1],
            m_indices[3*i+2]
        };
        for(size_t j=0; j<3; j++) {
            m_vertices[m_indices[3*i+j]].tangent = getTangent(
                triIndices[j],
                triIndices[(j+1)%3],
                triIndices[(j+2)%3]
            );
        }
    }
}

glm::vec3 Mesh::getTangent(uint32_t i0, uint32_t i1, uint32_t i2) {
    float u1 = m_vertices[i1].texCoord.x - m_vertices[i0].texCoord.x;
    float u2 = m_vertices[i2].texCoord.x - m_vertices[i0].texCoord.x;
    float v1 = m_vertices[i1].texCoord.y - m_vertices[i0].texCoord.y;
    float v2 = m_vertices[i2].texCoord.y - m_vertices[i0].texCoord.y;
    float invDenom = 1.0f / ((u1*v2) - (u2*v1));
    float tx = ((v1-v2) * m_vertices[i0].position.x + v2 * m_vertices[i1].position.x - v1 * m_vertices[i2].position.x) * invDenom;
    float ty = ((v1-v2) * m_vertices[i0].position.y + v2 * m_vertices[i1].position.y - v1 * m_vertices[i2].position.y) * invDenom;
    float tz = ((v1-v2) * m_vertices[i0].position.z + v2 * m_vertices[i1].position.z - v1 * m_vertices[i2].position.z) * invDenom;
    //float bx = ((u2-u1) * m_vertices[i0].position.x - u2 * m_vertices[i1].position.x + u1 * m_vertices[i2].position.x) * invDenom;
    //float by = ((u2-u1) * m_vertices[i0].position.y - u2 * m_vertices[i1].position.y + u1 * m_vertices[i2].position.y) * invDenom;
    //float bz = ((u2-u1) * m_vertices[i0].position.z - u2 * m_vertices[i1].position.z + u1 * m_vertices[i2].position.z) * invDenom;
    glm::vec3 tangent = glm::vec3(tx, ty, tz);
    auto normal = m_vertices[i0].normal;
    return glm::normalize(tangent - normal * glm::dot(normal, tangent));
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
    auto indexSize = static_cast<VkDeviceSize>(m_indices.size() * sizeof(uint32_t));
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
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, m_indices.size(), numInstances, 0, 0, 0);
}

void Mesh::cleanUp(std::shared_ptr<Context> &context) {
    vkDestroyBuffer(context->getDevice(), m_vertexBuffer, nullptr);
    vkFreeMemory(context->getDevice(), m_vertexMemory, nullptr);
    vkDestroyBuffer(context->getDevice(), m_indexBuffer, nullptr);
    vkFreeMemory(context->getDevice(), m_indexMemory, nullptr);

    m_hasBuffers = false;
}