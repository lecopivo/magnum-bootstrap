cmake_minimum_required(VERSION 2.8.12)
project(MyApplication)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/modules/")

set (CMAKE_CXX_STANDARD 17)

find_package(Magnum REQUIRED
  MeshTools
  Primitives
  Shaders
  SceneGraph
  Sdl2Application)

add_subdirectory(src)
