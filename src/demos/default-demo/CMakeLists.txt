cmake_minimum_required(VERSION 3.19)
project(slbDefaultDemo)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PUBLIC main.cpp)

#link external libraries
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES} glfw ${GLFW_LIBRARIES} glm::glm)

#add internal library
target_link_libraries(${PROJECT_NAME} PUBLIC slbLib)
target_include_directories(${PROJECT_NAME} PUBLIC "${LIB_DIR}")