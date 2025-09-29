#ifndef SLBVULKAN_RESOURCELOADER_H
#define SLBVULKAN_RESOURCELOADER_H

#include <string>
#include<vector>
#include <cstdint>
#include <fstream>
#include <iostream>

#include "path_config.h"

class ResourceLoader {
public:
    //loading a shader

    /**
     * Read a text file and return the contents.
     * 
     * Since this is used to read the compiled shaders the files are assumed to be in resources/shaders/spir-v.
     * 
     * @param fileName name of a file in the resources/shaders/spir-v folder
     * @return list of characters making up the file text
     */
    static std::vector<char> loadFile(const std::string &fileName);

    /**
     * Gather the indices of all descriptor sets mentioned in a shader.
     * 
     * Resources marked with "#include ..." are assumed to be required.
     * The indices stored for the descriptor sets are relative to the total descriptor set list in the Renderer class.
     * 
     * @param fileName name of a file in the resources/shaders folder
     * @param[out] requiredDescriptorSets list of indices of the descriptor sets required by all shaders in the shader set
     */
    static void findRequiredDescriptorSets(const std::string &fileName, std::vector<uint32_t> &requiredDescriptorSets);

    /**
     * Convert a descriptor name to the index of the set containing the descriptor.
     * 
     * The resulting index is relative to the total descriptor set list in the Renderer class.
     * 
     * @param descriptorName unique name identifying the descriptor
     * @return absolute index of the descriptor set
     */
    static uint32_t getDescriptorSetIndex(std::string descriptorName);

    /**
     * Compile a shader from glsl to spir-v.
     * 
     * First the shader text is copied into a new file with the "#include ..." lines replaced by the correct resource definitions.
     * The resulting file, stored in resources/shaders/used, is then compiled into resources/shaders/spir-v.
     * The name of the file is modified to avoid multiple files with the same name.
     * 
     * @param fileName name of a shader file in the resources/shaders/ folder
     * @param requiredDescriptorSets list of indices of the descriptor sets required by all shaders in the shader set
     * @return modified name of the compiled shader file
     */
    static std::string compileShader(const std::string &fileName, std::vector<uint32_t> &requiredDescriptorSets);

    /**
     * Convert a descriptor name into the shader text defining the resource.
     * 
     * The resulting string can be used to replace lines starting with "#include ..." in the shader.
     * The index of the descriptor set has to be relative to the local descriptor set list of the shader set.
     * 
     * @param descriptorName unique name identifying the descriptor
     * @param setIndex relative index of the descriptor set
     * @return shader code defining the descriptor
     */
    static std::string getDescriptorText(std::string descriptorName, uint32_t setIndex);
};

#endif //SLBVULKAN_RESOURCELOADER_H