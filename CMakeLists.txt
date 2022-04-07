cmake_minimum_required(VERSION 3.8)

project("IndividualProject")
set(CMAKE_CXX_STANDARD 11)

add_subdirectory("dependencies/glfw-3.3.6")

set(EMBREE_TASKING_SYSTEM OFF)
set(EMBREE_ISPC_SUPPORT OFF)
set(EMBREE_TUTORIALS OFF)

add_subdirectory("dependencies/embree-3.13.2")

set(LIBS embree)
set(INCLUDES "dependencies/embree-3.13.2/include")
set(KDTREE "dependencies/cdalitz-kdtree-cpp/kdtree.hpp" "dependencies/cdalitz-kdtree-cpp/kdtree.cpp")

set(HEADERS source/IOManagers/MeshGeometry.hpp source/IOManagers/PPMWriter.hpp source/Renderer/PointLight.hpp source/Renderer/RenderManager.hpp source/Renderer/PhotonMapper.hpp)
set(SOURCES source/IOManagers/MeshGeometry.cpp source/IOManagers/PPMWriter.cpp source/Renderer/PointLight.cpp source/Renderer/RenderManager.cpp source/Renderer/PhotonMapper.cpp)

#add_executable(HelloEmbree source/HelloEmbree.cpp)
#add_executable(AsciiTriangles source/AsciiTriangles.cpp ${HEADERS} ${SOURCES} ${KDTREE})
add_executable(MugScene source/MugScene.cpp ${HEADERS} ${SOURCES} ${KDTREE})
#add_executable(MainScene source/Main.cpp ${HEADERS} ${SOURCES} ${KDTREE})
add_executable(FinalScene source/FinalScene.cpp ${HEADERS} ${SOURCES} ${KDTREE})

#target_link_libraries(HelloEmbree ${LIBS})
#target_include_directories(HelloEmbree PUBLIC ${INCLUDES})

#target_link_libraries(AsciiTriangles ${LIBS})
#target_include_directories(AsciiTriangles PUBLIC ${INCLUDES})

target_link_libraries(MugScene ${LIBS})
target_include_directories(MugScene PUBLIC ${INCLUDES})

#target_link_libraries(MainScene ${LIBS})
#target_include_directories(MainScene PUBLIC ${INCLUDES})

target_link_libraries(FinalScene ${LIBS})
target_include_directories(FinalScene PUBLIC ${INCLUDES})