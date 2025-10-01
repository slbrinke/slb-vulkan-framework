#include "ResourceLoader.h"

std::vector<char> ResourceLoader::loadFile(const std::string &fileName) {
    std::ifstream file("../resources/shaders/spir-v/" + fileName, std::ios::ate | std::ios::binary);
    if(!file.is_open()) {
        throw std::runtime_error("RESOURCE LOADER ERROR: Could not read file: " + fileName);
    }

    auto fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    std::cout << "   RESOURCE LOADER: Loaded file " << fileName << " with size " << buffer.size() << std::endl;

    return buffer;
}

void ResourceLoader::findRequiredDescriptorSets(const std::string &fileName, std::vector<uint32_t> &requiredDescriptorSets) {
    std::ifstream file("../resources/shaders/" + fileName, std::ios::ate | std::ios::binary);
    if(!file.is_open()) {
        throw std::runtime_error("RESOURCE LOADER ERROR: Could not read file: " + fileName);
    }

    file.seekg(0);
    std::string line;
    while(std::getline(file, line)) {
        if(line.substr(0, 8) == "#include") {
            auto descriptorName = line.substr(9, line.length() - 9);
            auto absoluteIndex = getDescriptorSetIndex(descriptorName);
            
            size_t setIndex = 0;
            while(setIndex < requiredDescriptorSets.size() && requiredDescriptorSets[setIndex] < absoluteIndex) {
                setIndex++;
            }
            if(setIndex == requiredDescriptorSets.size()) {
                requiredDescriptorSets.emplace_back(absoluteIndex);
            } else if(requiredDescriptorSets[setIndex] != absoluteIndex) {
                requiredDescriptorSets.insert(requiredDescriptorSets.begin() + setIndex, absoluteIndex);
            }
        }
    }
}

uint32_t ResourceLoader::getDescriptorSetIndex(std::string descriptorName) {
    if(descriptorName == "Camera" || descriptorName == "Renderer") {
        return 0;
    } else if(descriptorName == "Materials" || descriptorName == "Lights" || descriptorName == "Textures" || descriptorName == "SceneNodeConstants") {
        return 1;
    }
    
    throw std::runtime_error("RESOURCE LOADER ERROR: There is no descriptor with the name " + descriptorName);
}

std::string ResourceLoader::compileShader(const std::string &fileName, std::vector<uint32_t> &requiredDescriptorSets) {
    auto slashPos = fileName.find_last_of('/');
    auto periodPos = fileName.find_last_of('.');

    //load input file
    std::ifstream inputFile("../resources/shaders/" + fileName, std::ios::ate | std::ios::binary);
    if(!inputFile.is_open()) {
        throw std::runtime_error("RESOURCE LOADER ERROR: Could not read file: " + fileName);
    }

    //write to output file
    std::ofstream outputFile;
    auto isolatedName = fileName.substr(slashPos + 1, fileName.length() - slashPos - 1);
    outputFile.open("../resources/shaders/used/" + isolatedName, std::ios::out);

    inputFile.seekg(0);
    std::string line;
    while(std::getline(inputFile, line)) {
        if(!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        if(line.substr(0, 8) == "#include") {
            auto descriptorName = line.substr(9, line.length() - 9);
            auto absoluteIndex = getDescriptorSetIndex(descriptorName);

            uint32_t setIndex = 0;
            while(setIndex < requiredDescriptorSets.size() && requiredDescriptorSets[setIndex] < absoluteIndex) {
                setIndex++;
            }
            outputFile << getDescriptorText(descriptorName, setIndex);
            
        } else {
            outputFile << line << "\n";
        }
    }
    inputFile.close();
    outputFile.close();

    //name of the compiled spv file
    auto compiledName = fileName.substr(slashPos + 1, periodPos - slashPos - 1)
            + std::string(1, std::toupper(fileName[periodPos + 1])) + fileName.substr(periodPos + 2, fileName.length())
            + ".spv";

    //compile
    std::string command = std::string(SHADER_COMPILER) + " ";
    command = command + RESOURCE_DIR + "/shaders/used/" + isolatedName;
    command = command + " -o ";
    command = command + RESOURCE_DIR + "/shaders/spir-v/" + compiledName;
    system(command.c_str());

    return compiledName;
}

std::string ResourceLoader::getDescriptorText(std::string descriptorName, uint32_t setIndex) {
    if(descriptorName == "Camera") {
        return "layout(set = " + std::to_string(setIndex) + ", binding = 0) uniform CameraUniforms {\n"
        + "   mat4 view;\n"
        + "   mat4 projection;\n"
        + "}camera;\n\n";
    } else if(descriptorName == "Renderer") {
        return "layout(set = " + std::to_string(setIndex) + ", binding = 1) uniform RendererUniforms {\n"
        + "   float pi;\n"
        + "   float inversePi;\n"
        + "   float epsilon;\n"
        + "   float pad;\n"
        + "}renderer;\n\n"; 
    } else if(descriptorName == "Materials") {
        return std::string("struct Material {\n")
        + "   vec3 color;\n"
        + "   float roughness;\n"
        + "   float metallic;\n"
        + "   float specular;\n"
        + "   float specularTint;\n"
        + "   float sheen;\n"
        + "   float sheenTint;\n"
        + "   float translucency;\n"
        + "   int diffuseTextureIndex;\n"
        + "   int normalTextureIndex;\n"
        + "   int roughnessTextureIndex;\n"
        + "   int metallicTextureIndex;\n"
        + "   float pad1;\n"
        + "   float pad2;\n"
        + "};\n\n"
        + "layout(set = " + std::to_string(setIndex) + ", binding = 0) uniform MaterialUniforms {\n"
        + "   Material materials[5];\n"
        + "};\n\n";
    } else if(descriptorName == "Lights") {
        return std::string("struct Light {\n")
        + "   vec3 position;\n"
        + "   float range;\n"
        + "   vec3 direction;\n"
        + "   float cosSpotAngle;\n"
        + "   vec3 color;\n"
        + "   float intensity;\n"
        + "};\n\n"
        + "layout(set = " + std::to_string(setIndex) + ", binding = 1) uniform LightUniforms {\n"
        + "   Light lights[5];\n"
        + "};\n\n";
    } else if(descriptorName == "Textures") {
        return std::string("layout(set = " + std::to_string(setIndex) + ", binding = 2) uniform sampler2D materialTextures[2];");
    } else if(descriptorName == "SceneNodeConstants") {
        return std::string("layout(push_constant, std430) uniform SceneNodeConstants {\n")
        + "   mat4 model;\n"
        + "   uint materialIndex;\n"
        + "};\n\n";
    }

    throw std::runtime_error("RESOURCE LOADER ERROR: There is no descriptor with name " + descriptorName);
}

void ResourceLoader::loadModel(const std::string &fileName, std::unique_ptr<SceneNode> &parent) {
    //load materials first
    std::vector<std::shared_ptr<Material>> materials;

    size_t startPos, endPos;

    std::ifstream mtlFile("../resources/models/" + fileName + ".mtl", std::ios::ate | std::ios::binary);
    if(!mtlFile.is_open()) {
        throw std::runtime_error("RESOURCE LOADER ERROR: Could not read file: " + fileName + ".mtl");
    }

    mtlFile.seekg(0);
    std::string line;
    while(std::getline(mtlFile, line)) {
        if(line.substr(0, 6) == "newmtl") {
            materials.emplace_back(std::make_shared<Material>());
            materials.back()->setName(line.substr(7));

        } else if(line.substr(0, 2) == "Kd") {
            materials.back()->setColor(textToVec3(line.substr(3)));

        } else if(line.substr(0, 6) == "map_Kd") {
            startPos = line.find_last_of('/');
            materials.back()->setDiffuseTexture(line.substr(startPos+1));
        } else if(line.substr(0, 2) == "Ks") {
            auto specular = textToVec3(line.substr(3));
            materials.back()->setSpecular((specular.x + specular.y + specular.z) / 3.0f);

        } else if(line.substr(0, 2) == "Ns") {
            materials.back()->setRoughness(std::stof(line.substr(3)));

        } else if(line.substr(0, 6) == "map_Ns") {
            startPos = line.find_last_of('/');
            materials.back()->setRoughnessTexture(line.substr(startPos+1));
        }
    }
    mtlFile.close();

    std::ifstream objFile("../resources/models/" + fileName + ".obj", std::ios::ate | std::ios::binary);
    if(!objFile.is_open()) {
        throw std::runtime_error("RESOURCE LOADER ERROR: Could not read file: " + fileName + ".obj");
    }

    std::vector<glm::vec3> loadedPositions;
    std::vector<glm::vec3> loadedNormals;
    std::vector<glm::vec2> loadedTexCoords;
    size_t numVertsPerFace;
    uint32_t vertexOffset = 0;
    size_t indexStart, indexEnd;

    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::string> matNames;

    objFile.seekg(0);
    while(std::getline(objFile, line)) {
        if(line.substr(0, 1) == "o" || line.length() == 0) {
            meshes.emplace_back(std::make_shared<Mesh>());
            vertexOffset = 0;

        } else if(line.substr(0, 6) == "usemtl") {
            matNames.emplace_back(line.substr(7));

        } else if(line.substr(0, 2) == "vn") {
            loadedNormals.emplace_back(ResourceLoader::textToVec3(line.substr(3)));
            
        } else if(line.substr(0, 2) == "vt") {
            loadedTexCoords.emplace_back(ResourceLoader::textToVec2(line.substr(3)));

        } else if(line.substr(0, 1) == "v") {
            loadedPositions.emplace_back(ResourceLoader::textToVec3(line.substr(2)));

        } else if(line.substr(0, 1) == "f") {
            numVertsPerFace = 0;
            startPos = 2;
            while(startPos >= 2 && startPos < line.length()) {
                endPos = line.find(' ', startPos);
                auto indices = line.substr(startPos, glm::min(endPos, line.length())-startPos);

                indexStart = 0;
                indexEnd = indices.find('/', indexStart);
                auto posIndex = std::stof(indices.substr(indexStart, indexEnd-indexStart)) - 1;
                indexStart = indexEnd+1;
                indexEnd = indices.find('/', indexStart);
                auto texCoordIndex = std::stof(indices.substr(indexStart, indexEnd-indexStart)) - 1;
                indexStart = indexEnd+1;
                auto normalIndex = std::stof(indices.substr(indexStart, indices.length()-indexStart)) - 1;
                meshes.back()->addVertex(
                    loadedPositions[posIndex],
                    loadedNormals[normalIndex],
                    loadedTexCoords[texCoordIndex],
                    glm::vec3(0.0f)
                );

                numVertsPerFace++;
                startPos = endPos+1;
            }
            if(numVertsPerFace == 3) {
                meshes.back()->addIndex(vertexOffset);
                meshes.back()->addIndex(vertexOffset + 1);
                meshes.back()->addIndex(vertexOffset + 2);
                vertexOffset += 3;
            } else if(numVertsPerFace == 4) {
                meshes.back()->addIndex(vertexOffset);
                meshes.back()->addIndex(vertexOffset + 1);
                meshes.back()->addIndex(vertexOffset + 2);
                meshes.back()->addIndex(vertexOffset + 2);
                meshes.back()->addIndex(vertexOffset + 3);
                meshes.back()->addIndex(vertexOffset);
                vertexOffset += 4;
            }
        }
    }
    objFile.close();

    for(size_t m=0; m<meshes.size(); m++) {
        size_t matIndex = 0;
        while(matIndex < materials.size() && materials[matIndex]->getName() != matNames[m]) {
            matIndex++;
        }
        if(matIndex >= materials.size()) {
            throw std::runtime_error("RESOURCE LOADER ERROR: Could not assign a material to name " + matNames[m]);
        }
        
        auto sceneNode = std::make_unique<SceneNode>(meshes[m], materials[matIndex]);
        parent->addChild(sceneNode);
    }
}

glm::vec2 ResourceLoader::textToVec2(std::string text) {
    size_t startPos = 0;
    size_t endPos = text.find(' ', startPos);
    float x = std::stof(text.substr(startPos, endPos-startPos));
    startPos = endPos+1;
    float y = std::stof(text.substr(startPos, text.length()-startPos));
    return glm::vec2(x, y);
}

glm::vec3 ResourceLoader::textToVec3(std::string text) {
    size_t startPos = 0;
    size_t endPos = text.find(' ', startPos);
    float x = std::stof(text.substr(startPos, endPos-startPos));
    startPos = endPos+1;
    endPos = text.find(' ', startPos);
    float y = std::stof(text.substr(startPos, endPos-startPos));
    startPos = endPos+1;
    float z = std::stof(text.substr(startPos, text.length()-startPos));
    return glm::vec3(x, y, z);
}