#version 450

layout(location = 0) in vec3 passPositionCamera;
layout(location = 1) in vec3 passNormalCamera;
layout(location = 2) in vec2 passTexCoord;

#include Camera
#include Renderer
#include Materials
#include Lights
#include SceneCounts
#include Textures
#include SceneNodeConstants

layout(location = 0) out vec4 fragmentColor;

vec3 getLightAttenuation(vec3 positionCamera, uint lightIndex, out vec3 lightVector) {
    Light light = lights[lightIndex];
    float intensity = light.intensity;

    if(light.cosSpotAngle == 1.0) { //directional light
        lightVector = normalize(vec3(camera.view * vec4(-light.direction, 0.0)));
        
    } else { //point or spot light
        lightVector = vec3(camera.view * vec4(light.position, 1.0)) - positionCamera;

        //attenuation over distance
        intensity *= 1.0 - min(length(lightVector) / light.range, 1.0);
        lightVector = normalize(lightVector);

        if(light.cosSpotAngle > 0.0) { //spot light
            //attenuation within spot cone
            vec3 lightDirectionCamera = vec3(camera.view * vec4(light.direction, 0.0));
            intensity *= max(dot(-lightVector, lightDirectionCamera) - light.cosSpotAngle, 0.0) / (1.0 - light.cosSpotAngle);
        }
    }

    return intensity * light.color;
}

vec3 getDiffuseLighting(vec3 normalCamera, vec3 lightVector, vec3 viewVector, vec3 baseColor) {
    vec3 halfVector = normalize(lightVector + viewVector);
    float nl = max(dot(normalCamera, lightVector), 0.0);
    float nv = max(dot(normalCamera, viewVector), 0.0);
    float lh = max(dot(lightVector, halfVector), 0.0);

    vec3 matDiffuse = baseColor * renderer.inversePi;
    float FD = 0.5 + 2.0 * materials[materialIndex].roughness * lh * lh;
    matDiffuse *= (1.0 + (FD - 1.0) * pow(1.0 - nl, 5.0)) * (1.0 + (FD - 1.0) * pow(1.0 - nv, 5.0));

    vec3 F0 = materials[materialIndex].metallic * baseColor + (1.0 - materials[materialIndex].metallic) * vec3(0.04);
    vec3 F = F0 + (vec3(1.0) - F0) * pow(1.0 - lh, 5.0);
    matDiffuse *= vec3(1.0) - F;

    return nl * matDiffuse;
}

vec3 getSpecularLighting(vec3 normalCamera, vec3 lightVector, vec3 viewVector, vec3 baseColor) {
    vec3 halfVector = normalize(lightVector + viewVector);
    float nl = max(dot(normalCamera, lightVector), 0.0);
    float nv = max(dot(normalCamera, viewVector), 0.0);
    float nh = max(dot(normalCamera, halfVector), 0.0);
    float lh = max(dot(lightVector, halfVector), 0.0);

    float alpha = materials[materialIndex].roughness * materials[materialIndex].roughness;
    float tmp = nh * nh * (alpha * alpha - 1.0) + 1.0; 
    float D = alpha * alpha / (renderer.pi * tmp*tmp);
    vec3 F0 = materials[materialIndex].metallic * baseColor + (1.0 - materials[materialIndex].metallic) * vec3(0.04);
    vec3 F = F0 + (vec3(1.0) - F0) * pow(1.0 - lh, 5.0);
    float kDirect = 0.125 * (materials[materialIndex].roughness + 1.0) * (materials[materialIndex].roughness + 1.0);
    float G = nv / (nv * (1.0 - kDirect) + kDirect);
    vec3 matSpecular = D * G * (materials[materialIndex].specularTint * baseColor + (1.0 - materials[materialIndex].specularTint) * vec3(1.0));
    matSpecular *= F;
    matSpecular *= materials[materialIndex].specular / (4.0 * nl * nv + 0.0001);

    return nl * matSpecular;
}

vec3 getPBShading(vec3 normalCamera, vec3 lightVector, vec3 viewVector, vec3 baseColor, float roughness) {
    vec3 halfVector = normalize(lightVector + viewVector);
    float nl = max(dot(normalCamera, lightVector), 0.0);
    float nv = max(dot(normalCamera, viewVector), 0.0);
    float nh = max(dot(normalCamera, halfVector), 0.0);
    float lh = max(dot(lightVector, halfVector), 0.0);

    //diffuse component
    vec3 matDiffuse = baseColor * renderer.inversePi;
    float FD = 0.5 + 2.0 * roughness * lh * lh;
    matDiffuse *= (1.0 + (FD - 1.0) * pow(1.0 - nl, 5.0)) * (1.0 + (FD - 1.0) * pow(1.0 - nv, 5.0));

    //specular component
    float alpha = roughness * roughness;
    float tmp = nh * nh * (alpha * alpha - 1.0) + 1.0; 
    float D = alpha * alpha / (renderer.pi * tmp*tmp);
    vec3 F0 = materials[materialIndex].metallic * baseColor + (1.0 - materials[materialIndex].metallic) * vec3(0.04);
    vec3 F = F0 + (vec3(1.0) - F0) * pow(1.0 - lh, 5.0);
    matDiffuse *= vec3(1.0) - F;
    float kDirect = 0.125 * (roughness + 1.0) * (roughness + 1.0);
    float G = nv / (nv * (1.0 - kDirect) + kDirect);
    vec3 matSpecular = D * G * (materials[materialIndex].specularTint * baseColor + (1.0 - materials[materialIndex].specularTint) * vec3(1.0));
    matSpecular *= F;
    matSpecular *= materials[materialIndex].specular / (4.0 * nl * nv + 0.0001);

    //additional sheen
    vec3 matSheen = materials[materialIndex].sheen * pow(1.0 - lh, 5.0) * (materials[materialIndex].sheenTint * baseColor + (1.0 - materials[materialIndex].sheenTint) * vec3(1.0));

    return nl * (matDiffuse + matSpecular + matSheen);
}

void main() {
    Material material = materials[materialIndex];

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
    vec3 viewVector = normalize(-passPositionCamera);

    //ambient base
    vec3 matFinal = 0.1 * baseColor;
    
    for(uint l=0; l<sceneCounts[1]; l++) {
        vec3 lightVector;
        vec3 lightColor = getLightAttenuation(passPositionCamera, l, lightVector);

        matFinal += lightColor * getPBShading(normalCamera, lightVector, viewVector, baseColor, roughness);
    }

    fragmentColor = vec4(matFinal, 1.0);
}