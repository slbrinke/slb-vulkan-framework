#ifndef SLBVULKAN_MATERIAL_H
#define SLBVULKAN_MATERIAL_H

#include <limits>
#include <string>

#include <glm/glm.hpp>

/**
 * If the roughness value is close to 0 the specular highlight disappears
 * because area lights are not implemented.
 * So we clamp roughness to a minimum for now.
 */
const float minRoughness = 0.1f;

/**
 * GPU representation of a material.
 * 
 * Contains all relevant brdf parameters used for shading.
 * For each material an instance is added to the material uniform buffer.
 */
struct MaterialUniforms {
    glm::vec3 color;
    float roughness;
    float metallic;
    float specular;
    float specularTint;
    float sheen;
    float sheenTint;
    float translucency;
    int diffuseTextureIndex;
    int normalTextureIndex;
    int roughnessTextureIndex;
    float pad1;
    float pad2;
    float pad3;
};

/**
 * Material characterizing the surface of a rendered mesh.
 * 
 * Contains parameters used for shading.
 */
class Material {
public:
    /**
     * Create a material either with specified color and roughness or with default values.
     * 
     * @param color base color in rgb format
     * @param roughness parameter characterizing how matte or shiny the surface looks
     */
    Material(glm::vec3 color = glm::vec3(0.85f, 0.67f, 0.29f), float roughness = 0.7f);
    ~Material();

    /**
     * Check wether an index has been assigned.
     * 
     * If this is not the case the material has not been added to a scene yet.
     * 
     * @return true if the material has a valid index
     */
    bool hasIndex();

    /**
     * Return the index assigned to the material by a scene.
     * 
     * This works off the assumption that there is only one relevant scene.
     * 
     * @return index identifying the material in the scene
     */
    uint32_t getIndex();

    /**
     * Return the name describing the material.
     * 
     * If loaded from an mtl file the name is taken from there.
     * Alternatively the name can be given by the application.
     * 
     * @return name identifying the material
     */
    std::string getName();

    /**
     * Provide material data to be added to a uniform buffer.
     * 
     * @return MaterialUniforms instance containing all relevant brdf parameters
     */
    MaterialUniforms getUniformData();

    /**
     * Assign an index to the material.
     * 
     * During rendering this index can be used as a uniform.
     * 
     * @param index unique index identifying the material
     */
    void setIndex(uint32_t index);

    /**
     * Change the name of the material.
     * 
     * @param name new name describing the material
     */
    void setName(std::string name);

    /**
     * Change the base color of the material.
     * 
     * The 3-component vector is interpreted as an rgb-color with values within [0.0, 1.0].
     * 
     * @param color new base color in rgb format
     */
    void setColor(glm::vec3 color);

    /**
     * Change the base color of the material.
     * 
     * It is defined in rgb format with values within [0.0, 1.0].
     * 
     * @param r red-channel value of the new base color
     * @param g green-channel value of the new base color
     * @param b blue-channel value of the new base color
     */
    void setColor(float r, float g, float b);

private:
    uint32_t m_index = std::numeric_limits<uint32_t>::max(); /**< Unique index identifying the material in the scene */
    std::string m_name = "Unnamed Material"; /**< Unique name identifying the material e.g. in an .mtl file */

    //material properties used as brdf parameters
    glm::vec3 m_color; /**< Base color of the material */
    float m_roughness; /**< Roughness ranging from shiny to matte */
    float m_metallic = 0.0f; /**< Parameter used for metal look */
    float m_specular = 1.0f; /**< Strength of the specular highlight */
    float m_specularTint = 0.0f; /**< Parameter mixing the base color into the specular highlight */
    float m_sheen = 0.0f; /**< Additional sheen effect for things like fabric */
    float m_sheenTint = 0.0f; /**< Parameter mixing the base color into the sheen component */
    float m_translucency = 0.0f; /**< Maximum thickness at which light shines through an object */
    
};

#endif //SLBVULKAN_MATERIAL_H