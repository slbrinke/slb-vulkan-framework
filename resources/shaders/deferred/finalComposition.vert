#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

#include Camera

layout(location = 0) out vec2 passTexCoord;

void main() {
    vec3 positionCamera = vec3(camera.view * vec4(vec3(inPosition), 0.0));
    gl_Position = camera.projection * vec4(positionCamera, 1.0);
    passTexCoord = inTexCoord;

}