#version 450

layout(points) in;
layout(line_strip, max_vertices = 50) out;

layout(location = 0) in vec3 passPositionWorld[];
layout(location = 1) in ivec3 passVoxelIndex[];
layout(location = 2) in uint passListIndex[];

#include Camera
#include Simulation
#include PlantModules
#include VoxelCount
#include VoxelData

layout(location = 0) out vec3 passColor;

mat4 getModelMatrix(vec3 pos, vec4 rot, float scale) {
    //scale
    mat4 mS = scale * mat4(1.0);
    mS[3][3] = 1.0;
    //rotate
    mat4 mR = transpose(mat4(
        1.0 - 2.0*rot.y*rot.y - 2.0*rot.z*rot.z, 2.0*rot.x*rot.y - 2.0*rot.w*rot.z, 2.0*rot.x*rot.z + 2.0*rot.w*rot.y, 0.0,
        2.0*rot.x*rot.y + 2.0*rot.w*rot.z, 1.0 - 2.0*rot.x*rot.x - 2.0*rot.z*rot.z, 2.0*rot.y*rot.z - 2.0*rot.w*rot.x, 0.0,
        2.0*rot.x*rot.z - 2.0*rot.w*rot.y, 2.0*rot.y*rot.z + 2.0*rot.w*rot.x, 1.0 - 2.0*rot.x*rot.x - 2.0*rot.y*rot.y, 0.0,
        0.0, 0.0, 0.0, 1.0
    ));
    //translate
    mat4 mT = mat4(1.0);
    mT[3] = vec4(pos, 1.0);
    return mT * mR * mS;
}

void drawCube(vec3 position, vec3 size) {
    vec3 halfSize = 0.5 * size;
    gl_Position = camera.projection * camera.view * vec4(position + vec3(-halfSize.x, -halfSize.y, halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(halfSize.x, -halfSize.y, halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(halfSize.x, halfSize.y, halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(-halfSize.x, halfSize.y, halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(-halfSize.x, -halfSize.y, halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(-halfSize.x, -halfSize.y, -halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(halfSize.x, -halfSize.y, -halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(halfSize.x, halfSize.y, -halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(-halfSize.x, halfSize.y, -halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(-halfSize.x, -halfSize.y, -halfSize.z), 1.0);
    EmitVertex();
    EndPrimitive();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(halfSize.x, -halfSize.y, halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(halfSize.x, -halfSize.y, -halfSize.z), 1.0);
    EmitVertex();
    EndPrimitive();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(halfSize.x, halfSize.y, halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(halfSize.x, halfSize.y, -halfSize.z), 1.0);
    EmitVertex();
    EndPrimitive();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(-halfSize.x, halfSize.y, halfSize.z), 1.0);
    EmitVertex();
    gl_Position = camera.projection * camera.view * vec4(position + vec3(-halfSize.x, halfSize.y, -halfSize.z), 1.0);
    EmitVertex();
    EndPrimitive();
}

void main() {
    passColor = vec3(
        float(passVoxelIndex[0].x) / float(simulation.numVoxels.x-1),
        float(passVoxelIndex[0].y) / float(simulation.numVoxels.y-1),
        float(passVoxelIndex[0].z) / float(simulation.numVoxels.z-1));

    uint numContents = voxelCount[passListIndex[0]+1] - voxelCount[passListIndex[0]];

    if(numContents > 0) {
        //drawCube(passPositionWorld[0], 0.7 * simulation.voxelSize);

        for(int m=0; m<numContents; m++) {
            gl_Position = camera.projection * camera.view * vec4(passPositionWorld[0], 1.0);
            EmitVertex();
            uint moduleIndex = voxelData[voxelCount[passListIndex[0]]+m];
            vec3 modulePos = vec3(getModelMatrix(currModules[moduleIndex].position, currModules[moduleIndex].rotation, 1.0) * vec4(currModules[moduleIndex].center, 1.0));
            gl_Position = camera.projection * camera.view * vec4(modulePos, 1.0);
            EmitVertex();
            EndPrimitive();
        }
    }
}