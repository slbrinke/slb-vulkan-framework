#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

#include Camera
#include SceneNodeConstants

layout(location = 0) out vec3 passPositionCamera;
layout(location = 1) out vec3 passNormalCamera;
layout(location = 2) out vec2 passTexCoord;
layout(location = 3) out vec3 passTangentCamera;
layout(location = 4) out vec3 passBitangentCamera;

void main(){
    gl_Position = camera.projection * camera.view * model * inPosition;
    passPositionCamera = vec3(camera.view * model * inPosition);
    passNormalCamera = normalize(vec3(camera.view * model * vec4(inNormal, 0.0)));
    passTexCoord = inTexCoord;
    passTangentCamera = normalize(vec3(camera.view * model * vec4(inTangent, 0.0)));
    passBitangentCamera = normalize(cross(passNormalCamera, passTangentCamera));
}