cmake_minimum_required(VERSION 3.30)
project(fulog)
set(fulog-src-files
  include/fulog.hpp
)
add_library(fulog INTERFACE)
add_library(fulog::fulog ALIAS fulog)
target_sources(fulog ${fulog-src-files})
target_include_directories(fulog INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
set_target_properties(fulog PROPERTIES CXX_STANDARD 20)