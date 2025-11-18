#version 450

layout(location = 0) in vec3 passColor;

layout(location = 0) out vec4 fragmentColor;

void main() {
    fragmentColor = vec4(passColor, 1.0);
}