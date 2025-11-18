set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LIB_DIR ${CMAKE_SOURCE_DIR}/src/library)
set(DEMO_DIR ${CMAKE_SOURCE_DIR}/src/demos)
set(EXTERN_DIR ${CMAKE_SOURCE_DIR}/extern)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

#add external libraries:
include(ExternalLibrariesSetup)

#add local library
add_subdirectory(${LIB_DIR})

#create executables for the demos
add_subdirectory(${DEMO_DIR}/default-demo)
add_subdirectory(${DEMO_DIR}/plant-demo)