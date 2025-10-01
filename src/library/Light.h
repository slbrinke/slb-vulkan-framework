#ifndef SLBVULKAN_LIGHT_H
#define SLBVULKAN_LIGHT_H

#include <glm/ext.hpp>

#include <limits>

#include "Mesh.h"

/**
 * GPU representation of a light source.
 * Contains all light parameters in world coordinates.
 * For each light source an instance is added to the light uniform buffer.
 */
struct LightUniforms {
    glm::vec3 position;
    float range;
    glm::vec3 direction;
    float cosSpotAngle;
    glm::vec3 color;
    float intensity;
};

/**
 * Light source contributing to the lighting of the scene.
 * 
 * Directional lights are distinguished from spot lights by setting the cone opening angle to zero.
 * Point lights are distinguished from spot lights by setting the cone opening angle to 180°.
 */
class Light {
public:
    /**
     * Create a light source with specified position and direction.
     * 
     * The parameters are interpreted in local coordiantes of the SceneNode the light is attached to.
     * For a point light the direction is irrelevant.
     * For a spot light the cone is centered around the direction vector.
     * 
     * @param position location in local coordinates
     * @param direction orientation in local coordinates
     */
    Light(glm::vec3 position, glm::vec3 direction);

    ~Light() = default;

    /**
     * Check wether an index has been assigned.
     * 
     * If this is not the case the light has not been added to a scene yet.
     * 
     * @return true if the light has a valid index
     */
    bool hasIndex();

    /**
     * Return the index assigned to the light by a scene.
     * 
     * This works off the assumption that there is only one relevant scene.
     * 
     * @return index identifying the light in the scene
     */
    uint32_t getIndex();

    /**
     * Provide light data to be added to a uniform buffer.
     * 
     * @param model accumulated model matrix of the SceneNode the light source is assigned to
     * 
     * @return LightUniforms instance containing all relevant light properties
     */
    LightUniforms getUniformData(glm::mat4 model);

    /**
     * Assign an index to the light source.
     * 
     * During rendering this index can be used as a push constant.
     * 
     * @param index unique index identifying the light
     */
    void setIndex(uint32_t index);

    /**
     * Change the location of the light source.
     * 
     * @param position new position in local scene node coordinates
     */
    void setPosition(glm::vec3 position);

    /**
     * Change the orientation of the light source.
     * 
     * For a point light the direction is irrelevant.
     * For a spot light the cone is centered around the direction vector.
     * 
     * @param direction new direction in local scene node coordinates
     */
    void setDirection(glm::vec3 direction);

    /**
     * Change the range light from this source can reach.
     * 
     * Range is always interpreted in world coordinates.
     * 
     * @param range new maximum light range
     */
    void setRange(float range);

    /**
     * Change the opening angle of the light cone.
     * 
     * If it is set to 0.0 the light is interpreted as a directional light.
     * If it is set to 180.0 the light is interpreted as a point light
     * Otherwise it is a spot light.
     * 
     * @param degrees full spot opening angle in degrees
     */
    void setSpotAngle(float degrees);

    /**
     * Change the light color.
     * 
     * The 3-component vector is interpreted as an rgb-color with values within [0.0, 1.0].
     * 
     * @param color new light color in rgb format
     */
    void setColor(glm::vec3 color);

    /**
     * Change the light color.
     * 
     * It is defined in rgb format with values within [0.0, 1.0].
     * 
     * @param r red-channel value of the new light color
     * @param g green-channel value of the new light color
     * @param b blue-channel value of the new light color
     */
    void setColor(float r, float g, float b);

    /**
     * Change the light intensity.
     * 
     * @param intensity new light intensity
     */
    void setIntensity(float intensity);

private:
    uint32_t m_index = std::numeric_limits<uint32_t>::max(); /**< Unique index identifying the light source in the scene */

    glm::vec3 m_position; /**< Location of the light source in local scene node coordinates */
    glm::vec3 m_direction; /**< Orientation the light source in local scene node coordinates */

    float m_range = 1.0f; /**< Maximum distance the light can reach in world coordinates */
    float m_spotAngle = 60.0f; /**< Full opening angle of the spot cone in degrees */

    glm::vec3 m_color{1.0f}; /**< Light color */
    float m_intensity = 1.0f; /**< Light intensity */

    std::shared_ptr<Mesh> m_proxyMesh = nullptr; /**< Proxy geometry representing the space the light can reach */

};

#endif //SLBVULKAN_LIGHT_H