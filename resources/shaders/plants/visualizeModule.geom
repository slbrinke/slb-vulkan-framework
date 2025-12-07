#version 450

layout(points) in;
layout(line_strip, max_vertices = 30) out;

layout(location = 0) in vec3 passCenter[];
layout(location = 1) in float passRadius[];
layout(location = 2) in float passVigor[];
layout(location = 3) in mat4 passModuleModel[];

#include Camera
#include Renderer

layout(location = 0) out vec3 passColor;

void main() {
    passColor = passVigor[0] * vec3(0.27, 0.93, 0.29)
        + (1.0 - passVigor[0]) * vec3(0.91, 0.37, 0.0);

    vec3 center = vec3(camera.view * passModuleModel[0] * vec4(passCenter[0], 1.0));
    float radius = passRadius[0];
    int resolution = 30;
    float step = 2.0 * renderer.pi / float(resolution-1);
    for(int i=0; i<resolution; i++) {
        float angle = float(i) * step;
        gl_Position = camera.projection * vec4(center + radius * vec3(cos(angle), sin(angle), 0.0), 1.0);
        EmitVertex();
    }
    EndPrimitive();
    
    /*
    passColor = vec3(1.0, 0.0, 0.0);
    gl_Position = camera.projection * camera.view * passModuleModel[0] * vec4(0.0, 0.0, 0.0, 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * passModuleModel[0] * vec4(1.0, 0.0, 0.0, 1.0);
    EmitVertex();
    EndPrimitive();
    passColor = vec3(0.0, 1.0, 0.0);
    gl_Position = camera.projection * camera.view * passModuleModel[0] * vec4(0.0, 0.0, 0.0, 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * passModuleModel[0] * vec4(0.0, 1.0, 0.0, 1.0);
    EmitVertex();
    EndPrimitive();
    passColor = vec3(0.0, 0.0, 1.0);
    gl_Position = camera.projection * camera.view * passModuleModel[0] * vec4(0.0, 0.0, 0.0, 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * passModuleModel[0] * vec4(0.0, 0.0, 1.0, 1.0);
    EmitVertex();
    EndPrimitive();
    */
}