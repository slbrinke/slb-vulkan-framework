#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureCoord;
layout(location = 3) in vec3 inTangent;

#include Camera
#include SceneCounts
#include PlantSpecies
#include PlantModules

layout(location = 0) out vec3 passCenter;
layout(location = 1) out float passRadius;
layout(location = 2) out uint passStatus;
layout(location = 3) out float passVigor;
layout(location = 4) out mat4 passModuleModel;

mat4 getModelMatrix(vec3 pos, vec3 rot, float scale) {
    //scale
    mat4 mS = scale * mat4(1.0);
    mS[3][3] = 1.0;
    //rotate
    /*
    mat4 mR = transpose(mat4(
        1.0 - 2.0*rot.y*rot.y - 2.0*rot.z*rot.z, 2.0*rot.x*rot.y - 2.0*rot.w*rot.z, 2.0*rot.x*rot.z + 2.0*rot.w*rot.y, 0.0,
        2.0*rot.x*rot.y + 2.0*rot.w*rot.z, 1.0 - 2.0*rot.x*rot.x - 2.0*rot.z*rot.z, 2.0*rot.y*rot.z - 2.0*rot.w*rot.x, 0.0,
        2.0*rot.x*rot.z - 2.0*rot.w*rot.y, 2.0*rot.y*rot.z + 2.0*rot.w*rot.x, 1.0 - 2.0*rot.x*rot.x - 2.0*rot.y*rot.y, 0.0,
        0.0, 0.0, 0.0, 1.0
    ));
    */
    mat4 mR = transpose(mat4(
        cos(rot.y)*cos(rot.z), -cos(rot.y)*sin(rot.z), sin(rot.y), 0.0,
        cos(rot.z)*sin(rot.x)*sin(rot.y)+cos(rot.x)*sin(rot.z), cos(rot.x)*cos(rot.z)-sin(rot.x)*sin(rot.y)*sin(rot.z), -cos(rot.y)*sin(rot.x), 0.0,
        -cos(rot.x)*cos(rot.z)*sin(rot.y)+sin(rot.x)*sin(rot.z), cos(rot.z)*sin(rot.x)+cos(rot.x)*sin(rot.y)*sin(rot.z), cos(rot.x)*cos(rot.y), 0.0,
        0.0, 0.0, 0.0, 1.0
    ));
    //translate
    mat4 mT = mat4(1.0);
    mT[3] = vec4(pos, 1.0);
    return mT * mR * mS;
}

void main() {
    uint moduleIndex = gl_InstanceIndex;
    
    Module module = currModules[moduleIndex];
    if(module.status > 0) {

        mat4 moduleModel = getModelMatrix(module.position, module.rotation, 1.0);
        gl_Position = camera.projection * camera.view * moduleModel * vec4(module.center, 1.0);
        passCenter = module.center;
        passRadius = module.radius;
        passStatus = module.status;
        passVigor = module.vigor;
        passModuleModel = moduleModel;
    }
    
}