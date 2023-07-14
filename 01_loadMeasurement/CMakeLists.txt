cmake_minimum_required(VERSION 3.20.0)
project(Example_01 C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(Example_01 main.c)
target_link_libraries(Example_01 PRIVATE cuvis::c)