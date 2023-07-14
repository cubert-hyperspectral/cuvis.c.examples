cmake_minimum_required(VERSION 3.20.0)
project(Example_06 C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(Example_06 main.c)
target_link_libraries(Example_06 PRIVATE cuvis::c)