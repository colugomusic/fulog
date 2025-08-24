cmake_minimum_required(VERSION 3.30)
project(fulog)
add_library(fulog INTERFACE)
add_library(fulog::fulog ALIAS fulog)
target_sources(fulog INTERFACE
	FILE_SET HEADERS
	BASE_DIRS
		${CMAKE_CURRENT_SOURCE_DIR}/include
	FILES
		${CMAKE_CURRENT_SOURCE_DIR}/include/fulog.hpp
)
set_target_properties(fulog PROPERTIES CXX_STANDARD 20)
include(CMakePackageConfigHelpers)
install(TARGETS fulog EXPORT fulog-targets FILE_SET HEADERS DESTINATION include/fulog)
install(EXPORT fulog-targets FILE fulog-targets.cmake NAMESPACE fulog:: DESTINATION lib/cmake/fulog)
configure_package_config_file(
	"${CMAKE_CURRENT_LIST_DIR}/cmake/fulog-config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/fulog-config.cmake"
	INSTALL_DESTINATION lib/cmake/fulog
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fulog-config.cmake" DESTINATION lib/cmake/fulog)