cmake_minimum_required(VERSION 3.20.0)
project(01_loadMeasurement C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(01_loadMeasurement main.c)
target_link_libraries(01_loadMeasurement PRIVATE cuvis::c)