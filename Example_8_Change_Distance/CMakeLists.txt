cmake_minimum_required(VERSION 3.16.0)
project(Example_8_Change_Distance C)
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../cuvis.c/")
find_package(Cuvis 3.2 REQUIRED)
add_executable(Example_8_Change_Distance main.c)
target_link_libraries(Example_8_Change_Distance PRIVATE cuvis::c)