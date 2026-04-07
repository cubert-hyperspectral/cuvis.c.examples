cmake_minimum_required(VERSION 3.16.0)
project(Example_5_Record_Video C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(Example_5_Record_Video main.c)
target_link_libraries(Example_5_Record_Video PRIVATE cuvis::c)