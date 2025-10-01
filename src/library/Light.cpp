#include "Light.h"

Light::Light(glm::vec3 position, glm::vec3 direction) : m_position(position), m_direction(glm::normalize(direction)) {

}

bool Light::hasIndex() {
    return m_index != std::numeric_limits<uint32_t>::max();
}

uint32_t Light::getIndex() {
    return m_index;
}

LightUniforms Light::getUniformData(glm::mat4 model) {
    auto posWorld = glm::vec3(model * glm::vec4(m_position, 1.0f));
    auto dirWorld = glm::normalize(glm::vec3(model * glm::vec4(m_direction, 0.0f)));
    LightUniforms lightUniforms {
        posWorld,
        m_range,
        dirWorld,
        glm::cos(glm::radians(0.5f * m_spotAngle)),
        m_color,
        m_intensity
    };
    return lightUniforms;
}

void Light::setIndex(uint32_t index) {
    m_index = index;
}

void Light::setPosition(glm::vec3 position) {
    m_position = position;
}

void Light::setDirection(glm::vec3 direction) {
    m_direction = glm::normalize(direction);
}

void Light::setRange(float range) {
    m_range = range;
}

void Light::setSpotAngle(float degrees) {
    m_spotAngle = degrees;
}

void Light::setColor(glm::vec3 color) {
    m_color = color;
}

void Light::setColor(float r, float g, float b) {
    m_color = glm::vec3(r, g, b);
}

void Light::setIntensity(float intensity) {
    m_intensity = intensity;
}