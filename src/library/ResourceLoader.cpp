#include "ResourceLoader.h"

std::vector<char> ResourceLoader::loadFile(const std::string &fileName) {
    std::ifstream file("../resources/shaders/spir-v/" + fileName, std::ios::ate | std::ios::binary);
    if(!file.is_open()) {
        throw std::runtime_error("Could not read file: " + fileName);
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
        throw std::runtime_error("Could not read file: " + fileName);
    }

    file.seekg(0);
    std::string line;
    while(std::getline(file, line)) {
        if(line.substr(0, 8) == "#include") {
            auto descriptorName = line.substr(9, line.length() - 9);
            auto setIndex = getDescriptorSetIndex(descriptorName);
            requiredDescriptorSets.emplace_back(setIndex);
        }
    }
}

uint32_t ResourceLoader::getDescriptorSetIndex(std::string descriptorName) {
    if(descriptorName == "Camera") {
        return 0;
    }
    
    throw std::runtime_error("RESOURCE LOADER ERROR: There is no descriptor with the name " + descriptorName);
}

std::string ResourceLoader::compileShader(const std::string &fileName, std::vector<uint32_t> &requiredDescriptorSets) {
    auto slashPos = fileName.find_last_of('/');
    auto periodPos = fileName.find_last_of('.');

    //load input file
    std::ifstream inputFile("../resources/shaders/" + fileName, std::ios::ate | std::ios::binary);
    if(!inputFile.is_open()) {
        throw std::runtime_error("Could not read file: " + fileName);
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
    }
    return "//test";
}