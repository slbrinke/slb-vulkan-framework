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
    static std::vector<char> loadFile(const std::string &fileName);
    static std::string compileShader(const std::string &fileName);
};

#endif //SLBVULKAN_RESOURCELOADER_H