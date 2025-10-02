#version 450

layout(location = 0) flat in uint passLightIndex;

#include Camera
#include Renderer
#include Materials
#include Lights
#include SceneCounts
#include Textures
#include SceneNodeConstants
#include GBuffer

layout(location = 0) out vec4 fragmentColor;

bool reconstructPosition(out vec3 positionCamera) {
    float depth = subpassLoad(gBufferDepth, gl_SampleID).x;
    if(depth == 1.0) {
        return false;
    }

    vec2 screenPos = 2.0 * vec2(gl_FragCoord.x / camera.screenWidth, gl_FragCoord.y / camera.screenHeight) - vec2(1.0);
    //positionCamera.z = camera.projection[3][2] / (depth * camera.projection[2][3] - camera.projection[2][2]);
    //float w = camera.projection[2][3] * positionCamera.z;
    positionCamera.z = (depth * camera.projection[3][3] - camera.projection[3][2])
        / (camera.projection[2][2] - depth * camera.projection[2][3]);
    float w = camera.projection[2][3] * positionCamera.z + camera.projection[3][3];
    positionCamera.x = screenPos.x * w / camera.projection[0][0];
    positionCamera.y = screenPos.y * w / camera.projection[1][1];

    return true;
}

vec3 getLightAttenuation(vec3 positionCamera, out vec3 lightVector) {
    Light light = lights[passLightIndex];
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

void main() {
    //retrieve depth and use as general stencil
    vec3 positionCamera;
    if(!reconstructPosition(positionCamera)) {
        discard;
    }

    //get data from the other gbuffer attachments
    vec4 normalData = subpassLoad(gBufferNormals, gl_SampleID);
    vec3 normalCamera;
    normalCamera.x = 2.0 * normalData.x - 1.0;
    normalCamera.y = 2.0 * normalData.y - 1.0;
    normalCamera.z = 1.0 - normalCamera.x*normalCamera.x - normalCamera.y*normalCamera.y;
    normalCamera = normalize(normalCamera);

    //retrieve other brdf parameters
    vec4 matData1 = subpassLoad(gBufferMaterials1, gl_SampleID);
    vec4 matData2 = subpassLoad(gBufferMaterials2, gl_SampleID);
    vec3 baseColor = vec3(matData1);
    float roughness = matData1.w;
    float metallic = normalData.z;
    float specular = matData2.x;
    float specularTint = matData2.y;
    float sheen = matData2.z;
    float sheenTint = matData2.w;
    float translucency = normalData.w;

    //vectors used for shading
    vec3 lightVector;
    vec3 lightColor = getLightAttenuation(positionCamera, lightVector);
    vec3 viewVector = normalize(-positionCamera);
    vec3 halfVector = normalize(lightVector + viewVector);
    float nl = max(dot(normalCamera, lightVector), 0.0);
    float nv = max(dot(normalCamera, viewVector), 0.0);
    float nh = max(dot(normalCamera, halfVector), 0.0);
    float lh = max(dot(lightVector, halfVector), 0.0);

    //main shading
    vec3 matFinal = vec3(0.0);
    if(nl > 0.0) {
        vec3 matDiffuse = baseColor * renderer.inversePi;
        float FD = 0.5 + 2.0 * roughness * lh * lh;
        matDiffuse *= (1.0 + (FD - 1.0) * pow(1.0 - nl, 5.0)) * (1.0 + (FD - 1.0) * pow(1.0 - nv, 5.0));

        //specular component
        float alpha = roughness * roughness;
        float tmp = nh * nh * (alpha * alpha - 1.0) + 1.0; 
        float D = alpha * alpha / (renderer.pi * tmp*tmp);
        vec3 F0 = metallic * baseColor + (1.0 - metallic) * vec3(0.04);
        vec3 F = F0 + (vec3(1.0) - F0) * pow(1.0 - lh, 5.0);
        matDiffuse *= vec3(1.0) - F;
        float kDirect = 0.125 * (roughness + 1.0) * (roughness + 1.0);
        float G = nv / (nv * (1.0 - kDirect) + kDirect);
        vec3 matSpecular = D * G * (specularTint * baseColor + (1.0 - specularTint) * vec3(1.0));
        matSpecular *= F;
        matSpecular *= specular / (4.0 * nl * nv + 0.0001);

        //additional sheen
        vec3 matSheen = sheen * pow(1.0 - lh, 5.0) * (sheenTint * baseColor + (1.0 - sheenTint) * vec3(1.0));

        matFinal = nl * (matDiffuse + matSpecular + matSheen);
    }

    fragmentColor = vec4(matFinal * lightColor, 1.0);
}