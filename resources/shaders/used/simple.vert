#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoord;
layout(location = 3) in vec3 inTangent;

layout(set = 0, binding = 0) uniform CameraUniforms {
   mat4 view;
   mat4 projection;
}camera;

layout(push_constant, std430) uniform SceneNodeConstants {
   mat4 model;
   uint materialIndex;
};


void main(){
    gl_Position = camera.projection * camera.view * model * inPosition;
}
