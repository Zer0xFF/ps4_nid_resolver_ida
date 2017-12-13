cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JSONCPP_WITH_TESTS OFF )
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF)
if(UNIX AND NOT APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()
add_subdirectory(extern/jsoncpp EXCLUDE_FROM_ALL)

add_executable(ps4_nid main.cpp ps4.cpp)
target_link_libraries(ps4_nid PRIVATE jsoncpp_lib_static)

if(DEFINED IDA_SDK)
	include("extern/ida-cmake/cmake/IDA.cmake")
	list(APPEND ida_libraries jsoncpp_lib_static)
	add_ida_plugin(ps4_nid_ida ps4.cpp ida_plugin.cpp ida_settings.cpp)
	add_ida_plugin(ps4_nid_ida_settings ida_plugin_settings.cpp ida_settings.cpp)
endif()
