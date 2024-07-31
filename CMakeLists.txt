cmake_minimum_required(VERSION 3.12)

set(CMAKE_BUILD_TYPE Debug)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

project(co_http LANGUAGES CXX)

#add_compile_options(-Wall -Wextra -Werror=return-type)

find_package(fmt REQUIRED)

file(GLOB sources CONFIGURE_DEPENDS *.cpp)
foreach (source IN ITEMS ${sources})
    get_filename_component(name "${source}" NAME_WLE)
    add_executable(${name} ${name}.cpp)
    #if(NOT TARGET fmt::fmt)
    #    message(FATAL_ERROR "the lib with name: fmt::fmt is not exist.")
    #endif()
    target_link_libraries(${name} PRIVATE fmt::fmt)
endforeach()
