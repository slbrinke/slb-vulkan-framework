set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LIB_DIR ${CMAKE_SOURCE_DIR}/src/library)
set(DEMO_DIR ${CMAKE_SOURCE_DIR}/src/demos)
set(EXTERN_DIR ${CMAKE_SOURCE_DIR}/extern)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

#add external libraries:
include(ExternalLibrariesSetup)

#executable for a default demo
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PUBLIC ${DEMO_DIR}/default-demo/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${DEMO_DIR} ${LIB_DIR})

#link external libraries
target_include_directories(${PROJECT_NAME} PUBLIC "C:/VulkanSDK/1.3.250.1/Include")
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES} glfw ${GLFW_LIBRARIES} glm::glm)

#add internal library
add_subdirectory(${LIB_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC slbLib)
target_include_directories(${PROJECT_NAME} PUBLIC "${LIB_DIR}")