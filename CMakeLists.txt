cmake_minimum_required(VERSION 3.8)

project("IndividualProject")

add_subdirectory("dependencies/glfw-3.3.6")

set(EMBREE_TASKING_SYSTEM OFF)
set(EMBREE_ISPC_SUPPORT OFF)
set(EMBREE_TUTORIALS OFF)

add_subdirectory("dependencies/embree-3.13.2")