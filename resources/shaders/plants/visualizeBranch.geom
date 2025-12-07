#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 7) out;

layout(location = 0) in vec3 passSize[];
layout(location = 1) in uint passSpeciesIndex[];
layout(location = 2) in mat4 passNodeModel[];

#include Camera

layout(location = 0) out vec3 passColor;

void main() {
    passColor = vec3(0.81, 0.86, 0.73);
    if(passSize[0].x >= 1.0) {
        passColor = vec3(0.31, 0.21, 0.12);
    }

    float extent = passSize[0].y;
    float radius = passSize[0].z;
    mat4 nodeModel = passNodeModel[0];
    vec3 p0 = vec3(camera.view * nodeModel * vec4(0.0, 0.0, 0.0, 1.0));
    vec3 p1 = vec3(camera.view * nodeModel * vec4(0.0, extent, 0.0, 1.0));
    
    gl_Position = camera.projection * vec4(p0 + vec3(-radius, 0.0, 0.0), 1.0);
    EmitVertex();
    gl_Position = camera.projection * vec4(p0 + vec3(radius, 0.0, 0.0), 1.0);
    EmitVertex();
    gl_Position = camera.projection * vec4(p1 + vec3(radius, 0.0, 0.0), 1.0);
    EmitVertex();
    EndPrimitive();
    gl_Position = camera.projection * vec4(p1 + vec3(radius, 0.0, 0.0), 1.0);
    EmitVertex();
    gl_Position = camera.projection * vec4(p1 + vec3(-radius, 0.0, 0.0), 1.0);
    EmitVertex();
    gl_Position = camera.projection * vec4(p0 + vec3(-radius, 0.0, 0.0), 1.0);
    EmitVertex();
    EndPrimitive();

}