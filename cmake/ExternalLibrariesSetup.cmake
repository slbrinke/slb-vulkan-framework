#vulkan
set(ENV{VULKAN_SDK} "C:/VulkanSDK/1.3.250.1")
find_package(Vulkan REQUIRED)

#glfw
add_subdirectory(${EXTERN_DIR}/glfw)
include_directories(${EXTERN_DIR}/glfw/include)

#glm
add_subdirectory(${EXTERN_DIR}/glm)

#stb
include_directories(${EXTERN_DIR}/stb)
link_directories(${EXTERN_DIR}/stb)