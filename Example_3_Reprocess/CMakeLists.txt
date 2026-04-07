cmake_minimum_required(VERSION 3.16.0)
project(Example_3_Reprocess C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(Example_3_Reprocess main.c)
target_link_libraries(Example_3_Reprocess PRIVATE cuvis::c)