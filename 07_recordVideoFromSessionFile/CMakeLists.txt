cmake_minimum_required(VERSION 3.20.0)
project(07_recordVideoFromSessionFile C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(07_recordVideoFromSessionFile main.c)
target_link_libraries(07_recordVideoFromSessionFile PRIVATE cuvis::c)