cmake_minimum_required(VERSION 3.16.0)
project(06_recordVideo C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(06_recordVideo main.c)
target_link_libraries(06_recordVideo PRIVATE cuvis::c)