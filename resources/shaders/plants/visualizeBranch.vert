#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoord;
layout(location = 3) in vec3 inTangent;

#include Camera
#include SceneCounts
#include Nodes
#include PlantSpecies
#include PlantPrototypes
#include PlantModules

layout(location = 0) out vec3 passSize;
layout(location = 1) out float passAge;
layout(location = 2) out uint passSpeciesIndex;
layout(location = 3) out mat4 passNodeModel;

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

bool treeIndexToNodeModel(uint moduleIndex, uint treeIndex, out mat4 nodeModel) {
    uint speciesIndex = currModules[moduleIndex].speciesIndex;
    uint maxChildren = plantSpecies[speciesIndex].maxChildren;

    uint level = 0;
    uint base = 1;
    uint offset = treeIndex;
    while(offset >= base) {
        offset -= base;
        base *= maxChildren;
        level++;
    }

    vec3 position = currModules[moduleIndex].position;
    vec4 modRot = currModules[moduleIndex].rotation;
    vec3 xAxis = vec3(
        1.0 - 2.0*modRot.y*modRot.y - 2.0*modRot.z*modRot.z,
        2.0*modRot.x*modRot.y + 2.0*modRot.w*modRot.z,
        2.0*modRot.x*modRot.z - 2.0*modRot.w*modRot.y);
    vec3 yAxis = vec3(
        2.0*modRot.x*modRot.y - 2.0*modRot.w*modRot.z,
        1.0 - 2.0*modRot.x*modRot.x - 2.0*modRot.z*modRot.z,
        2.0*modRot.y*modRot.z + 2.0*modRot.w*modRot.x);
    vec3 zAxis = vec3(
        2.0*modRot.x*modRot.z + 2.0*modRot.w*modRot.y,
        2.0*modRot.y*modRot.z - 2.0*modRot.w*modRot.x,
        1.0 - 2.0*modRot.x*modRot.x - 2.0*modRot.y*modRot.y);
    float extent = plantSpecies[speciesIndex].maxExtent;
    //float radius = plantSpecies[speciesIndex].maxRadius;
    //float parentRadius = radius;
    float radius = plantSpecies[speciesIndex].minRadius;

    uint listIndex = plantPrototypes[currModules[moduleIndex].prototypeIndex].firstNode;
    for(uint l=0; l<level; l++) {
        base /= maxChildren;
        uint child = offset / base;
        offset -= child * base;

        if(child >= currNodes[listIndex].numChildren
            || currModules[moduleIndex].age < plantSpecies[speciesIndex].maxAge - currNodes[currNodes[listIndex].childIndices[child]].age) {
            return false;
        }
        listIndex = currNodes[listIndex].childIndices[child];

        position += extent * yAxis;
        extent *= plantSpecies[speciesIndex].sizeDecrease[child];
        //parentRadius = radius;
        //radius *= plantSpecies[speciesIndex].sizeDecrease[child];
        float theta = plantSpecies[speciesIndex].branchingThetas[child];
        float phi = plantSpecies[speciesIndex].branchingPhis[child];
        float sinTheta = sin(theta);
        if(theta > 0.001) {
            yAxis = normalize(
                cos(phi) * sinTheta * xAxis
                + cos(theta) * yAxis
                + sin(phi) * sinTheta * zAxis
                + plantSpecies[speciesIndex].gravitropism * vec3(0.0, -1.0, 0.0));
            xAxis = normalize(vec3(-yAxis.z, 0.0, yAxis.x));
            zAxis = normalize(cross(xAxis, yAxis));
        } else {
            xAxis = normalize(
                cos(phi) * xAxis
                + sin(phi) * zAxis);
            yAxis = normalize(yAxis + plantSpecies[speciesIndex].gravitropism * vec3(0.0, -1.0, 0.0));
            zAxis = normalize(cross(xAxis, yAxis));
            xAxis = normalize(cross(yAxis, zAxis));
        }
    }

    nodeModel = mat4(
            vec4(xAxis, 0.0),
            vec4(yAxis, 0.0),
            vec4(zAxis, 0.0),
            vec4(position, 1.0))
        * transpose(mat4(
            radius, 0.0, 0.0, 0.0,
            0.0, extent, 0.0, 0.0,
            0.0, 0.0, radius, 0.0,
            0.0, 0.0, 0.0, 1.0));
    return true;
}

void main() {
    uint maxNodesPerPrototype = currSceneCounts[9];
    uint moduleIndex = gl_InstanceIndex / maxNodesPerPrototype;
    uint nodeIndex = gl_InstanceIndex - moduleIndex * maxNodesPerPrototype;

    Module module = currModules[moduleIndex];
    if(module.status > 0) {
        mat4 nodeModel = mat4(1.0);
        if(!treeIndexToNodeModel(moduleIndex, nodeIndex, nodeModel)) {
            return;
        }

        gl_Position = camera.projection * camera.view * nodeModel * vec4(0.0, 0.0, 0.0, 1.0);
        passSize = vec3(plantSpecies[module.speciesIndex].minExtent, plantSpecies[module.speciesIndex].maxExtent, plantSpecies[module.speciesIndex].minRadius);
        passAge = module.age / plantSpecies[module.speciesIndex].maxAge;
        passSpeciesIndex = module.speciesIndex;
        passNodeModel = nodeModel;
    }
    
}