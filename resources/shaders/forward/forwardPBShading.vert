#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoord;
layout(location = 3) in vec3 inTangent;

#include Camera
#include SceneNodeConstants

layout(location = 0) out vec3 passPositionCamera;
layout(location = 1) out vec3 passNormalCamera;

void main(){
    gl_Position = camera.projection * camera.view * model * inPosition;
    passPositionCamera = vec3(camera.view * model * inPosition);
    passNormalCamera = normalize(vec3(camera.view * model * vec4(inNormal, 0.0)));
}