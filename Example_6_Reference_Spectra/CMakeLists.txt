cmake_minimum_required(VERSION 3.16.0)
project(Example_6_Reference_Spectra C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.6 REQUIRED)
add_executable(Example_6_Reference_Spectra main.c)
target_link_libraries(Example_6_Reference_Spectra PRIVATE cuvis::c)