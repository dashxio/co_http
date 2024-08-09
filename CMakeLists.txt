# 最小cmake版本
cmake_minimum_required(VERSION 3.12)

# 项目名称
project(co_http LANGUAGES CXX)

# 编译选项
message(STATUS "Using compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "MSVC: ${MSVC}")
if (MSVC)
    add_compile_options(/Zc:preprocessor /utf-8 /DNOMINMAX /D_USE_MATH_DEFINES /EHsc /bigobj)
else()
    if (WIN32)
        add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8)
    endif()
    add_compile_options(-Wall -Wextra -Werror=return-type)
endif()

# 编译配置
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

# 找库
find_package(fmt REQUIRED)
find_package(Boost REQUIRED COMPONENTS locale)

# 生成目标
file(GLOB sources CONFIGURE_DEPENDS server.cpp)
file(GLOB depends CONFIGURE_DEPENDs *.hpp *.h *.cpp)
list(REMOVE_ITEM depends server.cpp)
foreach (source IN ITEMS ${sources})
    get_filename_component(name "${source}" NAME_WLE)
    add_executable(${name} ${name}.cpp ${depends})
    #if(NOT TARGET fmt::fmt)
    #    message(FATAL_ERROR "the lib with name: fmt::fmt is not exist.")
    #endif()
    target_link_libraries(${name} PRIVATE fmt::fmt Boost::locale)
endforeach()
