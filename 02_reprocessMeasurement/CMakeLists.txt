cmake_minimum_required(VERSION 3.20.0)
project(Example_02 C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_SOURCE_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 EXACT REQUIRED)
add_executable(Example_02 main.c)
target_link_libraries(Example_02 PRIVATE cuvis::c)