#ifndef SLBVULKAN_RESOURCELOADER_H
#define SLBVULKAN_RESOURCELOADER_H

#include <string>
#include <cstdint>
#include <fstream>
#include <iostream>

#include "path_config.h"
#include "SceneNode.h"

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
     * Compile a shader from glsl to spir-v.
     * 
     * First the shader text is copied into a new file with the "#include ..." lines replaced by the correct resource definitions.
     * The resulting file, stored in resources/shaders/used, is then compiled into resources/shaders/spir-v.
     * The name of the file is modified to avoid multiple files with the same name.
     * 
     * @param fileName name of a shader file in the resources/shaders/ folder
     * @param requiredDescriptorSets list of indices of the descriptor sets required by all shaders in the shader set
     * @param sceneCounts numbers of different components in the scene
     * @return modified name of the compiled shader file
     */
    static std::string compileShader(const std::string &fileName, std::vector<uint32_t> &requiredDescriptorSets, std::vector<uint32_t> &sceneCounts);

    /**
     * Read an .obj and .mtl file and convert it to renderable meshes with materials.
     * 
     * Both an .obj and an -mtl file with the given name have to be located in the resources/models folder.
     * Each separate mesh in the file is stored in a new scene node added to parent.
     * 
     * @param fileName name of a pair of .obj and .mtl files in resources/models
     * @param parent scene node receiving the loaded geometry as children
     */
    static void loadModel(const std::string &fileName, std::unique_ptr<SceneNode> &parent);

private:

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
     * Convert a descriptor name into the shader text defining the resource.
     * 
     * The resulting string can be used to replace lines starting with "#include ..." in the shader.
     * The index of the descriptor set has to be relative to the local descriptor set list of the shader set.
     * 
     * @param descriptorName unique name identifying the descriptor
     * @param setIndex relative index of the descriptor set
     * @param sceneCounts numbers of different components in the scene
     * @return shader code defining the descriptor
     */
    static std::string getDescriptorText(std::string descriptorName, uint32_t setIndex, std::vector<uint32_t> &sceneCounts);

    /**
     * Convert the file text representing a 2-component vector into a glm vector.
     * 
     * The coordinates are assumed to be divided by a space character in the file text.
     * 
     * @param text section of the file text containing only the vector data
     */
    static glm::vec2 textToVec2(std::string text);

    /**
     * Convert the file text representing a 3-component vector into a glm vector.
     * 
     * The coordinates are assumed to be divided by a space character in the file text.
     * 
     * @param text section of the file text containing only the vector data
     */
    static glm::vec3 textToVec3(std::string text);

};

#endif //SLBVULKAN_RESOURCELOADER_H