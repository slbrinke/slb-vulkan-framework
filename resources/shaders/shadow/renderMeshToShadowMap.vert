#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoord;
layout(location = 3) in vec3 inTangent;

#include Lights
#include SceneNodeConstants

layout(location = 0) out int passLightIndex;

void main(){
    Light light = lights[gl_InstanceIndex];

    gl_Position = light.shadowProj * light.shadowView * model * inPosition;

    passLightIndex = gl_InstanceIndex;

}