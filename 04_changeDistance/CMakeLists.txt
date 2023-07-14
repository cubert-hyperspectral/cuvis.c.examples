cmake_minimum_required(VERSION 3.20.0)
project(Example_04 C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(Example_04 main.c)
target_link_libraries(Example_04 PRIVATE cuvis::c)