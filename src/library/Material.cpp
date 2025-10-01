#include "Material.h"

Material::Material(glm::vec3 color, float roughness) : m_color(color), m_roughness(roughness) {

}

Material::~Material() {

}

bool Material::hasIndex() {
    return m_index != std::numeric_limits<uint32_t>::max();
}

uint32_t Material::getIndex() {
    return m_index;
}

std::string Material::getName() {
    return m_name;
}

MaterialUniforms Material::getUniformData() {
    MaterialUniforms matUniforms {
        m_color,
        m_roughness,
        m_metallic,
        m_specular,
        m_specularTint,
        m_sheen,
        m_sheenTint,
        m_translucency,
        -1,
        -1,
        -1,
        0.0f, 0.0f, 0.0f //padding for now
    };
    return matUniforms;
}

void Material::setIndex(uint32_t index) {
    m_index = index;
}

void Material::setName(std::string name) {
    m_name = name;
}

void Material::setColor(glm::vec3 color) {
    m_color = color;
}

void Material::setColor(float r, float g, float b) {
    m_color = glm::vec3(r, g, b);
}

void Material::setRoughness(float roughness) {
    m_roughness = roughness;
}

void Material::setMetallic(float metallic) {
    m_metallic = metallic;
}

void Material::setSpecular(float specular) {
    m_specular = specular;
}

void Material::setSpecularTint(float tint) {
    m_specularTint = tint;
}

void Material::setSheen(float sheen) {
    m_sheen = sheen;
}

void Material::setSheenTint(float tint) {
    m_sheenTint = tint;
}