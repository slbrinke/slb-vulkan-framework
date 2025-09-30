#version 450

struct Material {
   vec3 color;
   float roughness;
   float metallic;
   float specular;
   float specularTint;
   float sheen;
   float sheenTint;
   float translucency;
   int diffuseTextureIndex;
   int normalTextureIndex;
   int roughnessTextureIndex;
   float pad1;
   float pad2;
   float pad3;
};

layout(set = 1, binding = 0) uniform MaterialUniforms {
   Material materials[10];
};

layout(push_constant, std430) uniform SceneNodeConstants {
   mat4 model;
   uint materialIndex;
};


layout(location = 0) out vec4 fragmentColor;

void main(){
    //'************************>AY
    //my dog stepped on the keyboard
    //it was so cute I had to leave it in

    vec3 baseColor = materials[materialIndex].color;
    fragmentColor = vec4(baseColor, 1.0);
}
