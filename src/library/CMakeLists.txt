cmake_minimum_required(VERSION 3.19)
project(slbLib)

set(LIB_SOURCES
        ${LIB_DIR}/Context.cpp
        ${LIB_DIR}/Context.h
)

add_library(slbLib STATIC ${LIB_SOURCES})

#external dependecies
target_include_directories(slbLib SYSTEM BEFORE PRIVATE "C:/VulkanSDK/1.3.250.1/Include")
target_include_directories(slbLib SYSTEM BEFORE PRIVATE
        ${EXTERN_DIR}/glfw/include
        ${EXTERN_DIR}/stb
        #${EXTERN_DIR}/imgui
        #${EXTERN_DIR}/imgui/backends
)
target_link_libraries(slbLib ${Vulkan_LIBRARIES} glfw ${GLFW_LIBRARIES} glm::glm)

target_include_directories(slbLib BEFORE PUBLIC ${LIB_DIR})