cmake_minimum_required(VERSION 3.20.0)
project(04_changeDistance C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(04_changeDistance main.c)
target_link_libraries(04_changeDistance PRIVATE cuvis::c)