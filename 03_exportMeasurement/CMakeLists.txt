cmake_minimum_required(VERSION 3.16.0)
project(03_exportMeasurement C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(03_exportMeasurement main.c)
target_link_libraries(03_exportMeasurement PRIVATE cuvis::c)