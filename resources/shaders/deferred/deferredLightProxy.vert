#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

#include Camera
#include Lights
#include SceneNodeConstants

layout(location = 0) out uint passLightIndex;

void main() {
    if(lights[currentIndex].cosSpotAngle == 1.0) { //directional light
        gl_Position = inPosition;
    } else {
        gl_Position = camera.projection * camera.view * model * inPosition;
    }
    passLightIndex = currentIndex;
    
}