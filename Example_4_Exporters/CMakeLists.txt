cmake_minimum_required(VERSION 3.16.0)
project(Example_4_Exporters C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.5 REQUIRED)
add_executable(Example_4_Exporters main.c)
target_link_libraries(Example_4_Exporters PRIVATE cuvis::c)