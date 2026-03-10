#version 450

layout(location = 0) in vec2 passTexCoord;

#include Renderer
#include Textures
#include ShadingResult

layout(location = 0) out vec4 fragmentColor;

vec3 convertSRGBToLinear(vec3 srgb) {
    vec3 linear = vec3(0.0, 0.0, 0.0);
    if(srgb.x <= 0.04045) {
        linear.x = srgb.x / 12.92;
    } else {
        linear.x = pow((srgb.x + 0.055) / 1.055, 2.4);
    }
    if(srgb.y <= 0.04045) {
        linear.y = srgb.y / 12.92;
    } else {
        linear.y = pow((srgb.y + 0.055) / 1.055, 2.4);
    }
    if(srgb.z <= 0.04045) {
        linear.z = srgb.z / 12.92;
    } else {
        linear.z = pow((srgb.z + 0.055) / 1.055, 2.4);
    }
    return linear;
}

void main() {
    vec3 final = vec3(0.0, 0.0, 0.0);

    vec4 shadingData = subpassLoad(shadingResult, gl_SampleID);
    if(shadingData.w > 0.0) {
        final = vec3(shadingData);
    } else if(renderer.useEnvMap) {
        vec2 envCoord = vec2(passTexCoord.x + renderer.envMapOffset, passTexCoord.y);
        final = vec3(texture(materialTextures[0], envCoord));
    }

    fragmentColor = vec4(convertSRGBToLinear(final), 1.0);

}