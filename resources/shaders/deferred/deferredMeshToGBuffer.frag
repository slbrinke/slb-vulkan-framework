#version 450

layout(location = 0) in vec3 passPositionCamera;
layout(location = 1) in vec3 passNormalCamera;
layout(location = 2) in vec2 passTexCoord;
layout(location = 3) in vec3 passTangentCamera;
layout(location = 4) in vec3 passBitangentCamera;

#include Camera
#include Renderer
#include Materials
#include Lights
#include SceneCounts
#include Textures
#include SceneNodeConstants

layout(location = 0) out vec4 normalOutput;
layout(location = 1) out vec4 matOutput1;
layout(location = 2) out vec4 matOutput2;

void main() {
    Material material = materials[currentIndex];

    vec3 baseColor = material.color;
    if(material.diffuseTextureIndex > -1) {
        vec4 diffuseData = texture(materialTextures[material.diffuseTextureIndex], passTexCoord);
        baseColor = vec3(diffuseData);
    }

    float roughness = material.roughness;
    if(material.roughnessTextureIndex > -1) {
        roughness = texture(materialTextures[material.roughnessTextureIndex], passTexCoord).x;
    }

    vec3 normalCamera = passNormalCamera;
    if(material.normalTextureIndex > -1) {
        vec4 normalData = texture(materialTextures[material.normalTextureIndex], passTexCoord);
        normalCamera = normalize(
            (2.0 * normalData.x - 1.0) * passTangentCamera
            + (2.0 * normalData.y - 1.0) * passBitangentCamera
            + (2.0 * normalData.z - 1.0) * passNormalCamera);
    }
    
    normalOutput = vec4(
        0.5 * normalCamera.x + 0.5,
        0.5 * normalCamera.y + 0.5,
        material.metallic,
        material.translucency
    );
    matOutput1 = vec4(baseColor, roughness);
    matOutput2 = vec4(material.specular, material.specularTint, material.sheen, material.sheenTint);
}