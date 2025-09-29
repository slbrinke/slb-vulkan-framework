cmake_minimum_required(VERSION 3.19)
project(slbLib)

set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(SHADER_COMPILER ${Vulkan_GLSLC_EXECUTABLE})
configure_file(${LIB_DIR}/path_config.h.in ${LIB_DIR}/path_config.h)

set(LIB_SOURCES
        ${LIB_DIR}/Camera.cpp
        ${LIB_DIR}/Camera.h
        ${LIB_DIR}/Context.cpp
        ${LIB_DIR}/Context.h
        ${LIB_DIR}/DescriptorSet.cpp
        ${LIB_DIR}/DescriptorSet.h
        ${LIB_DIR}/Image.cpp
        ${LIB_DIR}/Image.h
        ${LIB_DIR}/Mesh.cpp
        ${LIB_DIR}/Mesh.h
        ${LIB_DIR}/path_config.h
        ${LIB_DIR}/Renderer.cpp
        ${LIB_DIR}/Renderer.h
        ${LIB_DIR}/RenderOutput.cpp
        ${LIB_DIR}/RenderOutput.h
        ${LIB_DIR}/RenderStep.cpp
        ${LIB_DIR}/RenderStep.h
        ${LIB_DIR}/ResourceLoader.cpp
        ${LIB_DIR}/ResourceLoader.h
        ${LIB_DIR}/Scene.cpp
        ${LIB_DIR}/Scene.h
)

add_library(slbLib STATIC ${LIB_SOURCES})

#external dependecies
target_include_directories(slbLib SYSTEM BEFORE PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(slbLib SYSTEM BEFORE PRIVATE
        ${EXTERN_DIR}/glfw/include
        ${EXTERN_DIR}/stb
)
target_link_libraries(slbLib ${Vulkan_LIBRARIES} glfw ${GLFW_LIBRARIES} glm::glm)

target_include_directories(slbLib BEFORE PUBLIC ${LIB_DIR})