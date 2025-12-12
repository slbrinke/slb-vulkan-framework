#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoord;
layout(location = 3) in vec3 inTangent;

#include Camera
#include Simulation

layout(location = 0) out vec3 passPositionWorld;
layout(location = 1) out ivec3 passVoxelIndex;
layout(location = 2) out uint passListIndex;

uint voxelToListIndex(ivec3 voxelIndex) {
    uint listIndex = 0;
    listIndex += voxelIndex.x;
    listIndex += voxelIndex.y * simulation.numVoxels.x;
    listIndex += voxelIndex.z * (simulation.numVoxels.x * simulation.numVoxels.y);
    return listIndex;
}

ivec3 listToVoxelIndex(uint listIndex) {
    ivec3 voxelIndex = ivec3(0, 0, 0);
    uint remainder = listIndex;
    voxelIndex.z = int(remainder / uint(simulation.numVoxels.x * simulation.numVoxels.y));
    remainder -= uint(voxelIndex.z * simulation.numVoxels.x * simulation.numVoxels.y);
    voxelIndex.y = int(remainder / uint(simulation.numVoxels.x));
    remainder -= uint(voxelIndex.y * simulation.numVoxels.x);
    voxelIndex.x = int(remainder);
    return voxelIndex;
}

void main() {
    uint listIndex = gl_InstanceIndex;
    ivec3 voxelIndex = listToVoxelIndex(listIndex);
    
    vec3 voxelPos = -0.5 * simulation.sceneSize + vec3(float(voxelIndex.x) + 0.5, float(voxelIndex.y) + 0.5, float(voxelIndex.z) + 0.5) * simulation.voxelSize;
    gl_Position = camera.projection * camera.view * vec4(voxelPos, 1.0);
    passPositionWorld = voxelPos;
    passVoxelIndex = voxelIndex;
    passListIndex = listIndex;
    
}