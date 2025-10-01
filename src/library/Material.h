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
    int metallicTextureIndex;
    float pad1;
    float pad2;
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
     * Check if there is a diffuse texture assigned to the material.
     * 
     * @return true if the material has a diffuse texture
     */
    bool hasDiffuseTexture();

    /**
     * Return the file name of the image used as diffuse texture.
     * 
     * Once loaded the texture is used as material base color.
     * 
     * @return name of the diffuse texture file
     */
    std::string getDiffuseTexture();

    /**
     * Check if there is a normal map assigned to the material.
     * 
     * @return true if the material has a normal texture
     */
    bool hasNormalTexture();

    /**
     * Return the file name of the image used as normal texture.
     * 
     * Once loaded the texture is used as normal map.
     * 
     * @return name of the normal texture file
     */
    std::string getNormalTexture();

    /**
     * Check if there is a roughness texture assigned to the material.
     * 
     * @return true if the material has a roughness texture
     */
    bool hasRoughnessTexture();

    /**
     * Return the file name of the image used as a roughness texture.
     * 
     * Once loaded the texture is used as material roughness.
     * 
     * @return name of the roughness texture file
     */
    std::string getRoughnessTexture();

    /**
     * Check if there is a metallic texture assigned to the material.
     * 
     * @return true if the material has a metallic texture
     */
    bool hasMetallicTexture();

    /**
     * Return the file name of the image used as a metallic texture.
     * 
     * Once loaded the texture is used as metallic parameter.
     * 
     * @return name of the metallic texture file
     */
    std::string getMetallicTexture();

    /**
     * Assign an index to the material.
     * 
     * During rendering this index can be used as a push constant.
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

    /**
     * Change the roughness parameter.
     * 
     * It can technically take on values between 0.0 (shiny) and 1.0 (matte).
     * But since area lights are not implemented here small values are clamped to a minimum.
     * 
     * @param roughness new roughness value
     */
    void setRoughness(float roughness);

    /**
     * Change the metallic parameter.
     * 
     * The higher the value the more the surface looks like metal.
     * Per default it is 0.0 and the maximum is 1.0.
     * 
     * @param metallic new metallic value
     */
    void setMetallic(float metallic);

    /**
     * Change the specular parameter.
     * 
     * It scales the strength of the specular highlight.
     * Per default it is 1.0 and for 0.0 the highlight disappears.
     * 
     * @param specular new specular value
     */
    void setSpecular(float specular);

    /**
     * Change the specular tint parameter.
     * 
     * It scales how strongly the base color is mixed into the specular highlight.
     * Per default it is 0.0 and the maximum is 1.0.
     * 
     * @param tint new specular tint value
     */
    void setSpecularTint(float tint);

    /**
     * Change the sheen parameter.
     * 
     * It scales an optional grazing component.
     * Per default it is 0.0 and the maximum is 1.0.
     * 
     * @param sheen new sheen value
     */
    void setSheen(float sheen);

    /**
     * Change the sheen tint parameter.
     * 
     * It scales how strongly the base color is mixed into the sheen component.
     * Per default it is 0.0 and the maximum is 1.0.
     * 
     * @param tint new sheen tint value
     */
    void setSheenTint(float tint);

    /**
     * Add a texture as material base color.
     * 
     * The image itself is loaded later when the scene is initialized.
     * For now the material only stores the file name.
     * 
     * @param fileName name of an image file in resources/textures
     */
    void setDiffuseTexture(std::string fileName);

    /**
     * Add a texture as normal map.
     * 
     * The image itself is loaded later when the scene is initialized.
     * For now the material only stores the file name.
     * 
     * @param fileName name of an image file in resources/textures
     */
    void setNormalTexture(std::string fileName);

    /**
     * Add a texture as material roughness.
     * 
     * The image itself is loaded later when the scene is initialized.
     * For now the material only stores the file name.
     * 
     * @param fileName name of an image file in resources/textures
     */
    void setRoughnessTexture(std::string fileName);

    /**
     * Add a texture as metallic parameter.
     * 
     * The image itself is loaded later when the scene is initialized.
     * For now the material only stores the file name.
     * 
     * @param fileName name of an image file in resources/textures
     */
    void setMetallicTexture(std::string fileName);

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

    std::string m_diffuseTextureFile = ""; /**< File containing the image used as a base color texture */
    std::string m_normalTextureFile = ""; /**< File containing the image used as a normal map */
    std::string m_roughnessTextureFile = ""; /**< File containing the image used as roughness map */
    std::string m_metallicTextureFile = ""; /**< File containing the image used a metallic map */
    
};

#endif //SLBVULKAN_MATERIAL_H