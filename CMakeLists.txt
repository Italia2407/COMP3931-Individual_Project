cmake_minimum_required(VERSION 3.8)

project("IndividualProject")

add_subdirectory("dependencies/glfw-3.3.6")

set(EMBREE_TASKING_SYSTEM OFF)
set(EMBREE_ISPC_SUPPORT OFF)
set(EMBREE_TUTORIALS OFF)

add_subdirectory("dependencies/embree-3.13.2")

set(LIBS embree)
set(INCLUDES "dependencies/embree-3.13.2/include")

add_executable(HelloEmbree source/HelloEmbree.cpp)
add_executable(AsciiTriangles source/AsciiTriangles.cpp source/RenderManager.cpp source/IOManagers/MeshGeometry.cpp source/IOManagers/PPMWriter.cpp)

target_link_libraries(HelloEmbree ${LIBS})
target_include_directories(HelloEmbree PUBLIC ${INCLUDES})

target_link_libraries(AsciiTriangles ${LIBS})
target_include_directories(AsciiTriangles PUBLIC ${INCLUDES})