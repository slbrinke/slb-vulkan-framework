#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoord;
layout(location = 3) in vec3 inTangent;

layout(set = 0, binding = 0) uniform CameraUniforms {
   mat4 view;
   mat4 projection;
}camera;


void main(){
    gl_Position = camera.projection * camera.view * inPosition;
}
