#version 450

#include Materials
#include SceneNodeConstants

layout(location = 0) out vec4 fragmentColor;

void main(){
    //'************************>AY
    //my dog stepped on the keyboard
    //it was so cute I had to leave it in

    vec3 baseColor = materials[materialIndex].color;
    fragmentColor = vec4(baseColor, 1.0);
}