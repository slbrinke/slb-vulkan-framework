#version 450

layout(location = 0) in vec2 passTexCoord;

#include Textures
#include ShadingResult

layout(location = 0) out vec4 fragmentColor;

void main() {
    vec3 final = vec3(0.0, 0.0, 0.0);

    vec4 shadingData = subpassLoad(shadingResult, gl_SampleID);
    if(shadingData.w > 0.0) {
        final = vec3(shadingData);
    } else {
        final = vec3(texture(materialTextures[0], passTexCoord));
    }

    fragmentColor = vec4(final, 1.0);

}