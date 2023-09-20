cmake_minimum_required(VERSION 3.16.0)
project(02_reprocessMeasurement C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(02_reprocessMeasurement main.c)
target_link_libraries(02_reprocessMeasurement PRIVATE cuvis::c)