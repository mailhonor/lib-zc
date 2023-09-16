cmake_minimum_required(VERSION 2.8.12)

project(lib-zc)

# 通用工具
include(./dev/cmake_general_utils.cmake)

# 跨操作系统平台相关
set(platform_include_dirs)
set(platform_lib_dirs)
set(platform_libs)

IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(platform_include_dirs C:/_dev/vcpkg-master/installed/x64-mingw-static/include C:/_dev/mingw64/x86_64-w64-mingw32/include)
	set(platform_lib_dirs C:/_dev/vcpkg-master/installed/x64-mingw-static/lib C:/_dev/mingw64/x86_64-w64-mingw32/lib)
	set(platform_libs Winmm Ws2_32 Crypt32 Iphlpapi)
ENDIF()

IF(EXISTS "${PROJECT_SOURCE_DIR}/dev/platform.cmake")
    include("${PROJECT_SOURCE_DIR}/dev/platform.cmake")
ENDIF()


set(build_target_auto_int_pname_idx 10)
set(build_target_auto_int_name_idx 10)
set(target_auto_int_name "1010")

function(build_target_auto_int_name)
    math(EXPR build_target_auto_int_name_idx "${build_target_auto_int_name_idx} + 1")
    set(build_target_auto_int_name_idx ${build_target_auto_int_name_idx} PARENT_SCOPE)
    set(target_auto_int_name ${build_target_auto_int_pname_idx}${build_target_auto_int_name_idx} PARENT_SCOPE)
endfunction()

build_target_auto_int_name()

# option(ENABLE_ICU_UCONV "icu charset convert" OFF)
# option(ENABLE_SQLITE "sqlite3 proxy server" OFF)
# 其他个性化设置
IF(EXISTS "${PROJECT_SOURCE_DIR}/dev/extra.cmake")
    include("${PROJECT_SOURCE_DIR}/dev/extra.cmake")
ENDIF()

set(my_compile_definitions
    "-D___ZC_DEV_MODE___"
    "-D_GNU_SOURCE"
    "-D_POSIX"
    "-g" "-ggdb"
    "-O2"
    "-Wall"
    "-Winline"
    "-Wcast-align"
    "-Wno-long-long"
    "-Wno-format-zero-length"
    "-Wpointer-arith"
    "-D_REENTRANT"
    "-D_POSIX_PTHREAD_SEMANTICS"
    "-D_USE_FAST_MACRO"
    "-Wno-unused"
    "-DLINUX2"
)

if(ENABLE_ICU_UCONV)
    list(APPEND my_compile_definitions "-D_LIB_ZC_UCONV_")
endif()

if(ENABLE_SQLITE)
    list(APPEND my_compile_definitions "-D_LIB_ZC_SQLITE3_")
endif()

set(CMAKE_C_FLAGS "-std=gnu99 ")
set(CMAKE_CXX_FLAGS "-std=gnu++11 -D___ZC_ZCC_MODE___")

FILE(GLOB COROUTINE_SOURCES "src/coroutine/*.c")
FILE(GLOB ALL_SOURCES "src/*/*.c" "cpp_src/*/*.cpp")
list(REMOVE_ITEM ALL_SOURCES ${COROUTINE_SOURCES})

INCLUDE_DIRECTORIES(
    ./
    ${platform_include_dirs}
)

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_EXE_LINKER_FLAGS -rdynamic)
ENDIF()

ADD_LIBRARY(zc STATIC ${ALL_SOURCES})
target_compile_options(zc PRIVATE ${my_compile_definitions})

zcc_problem_FILE_macro(zc)
set_target_properties(zc PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    ADD_LIBRARY(zc_coroutine STATIC ${COROUTINE_SOURCES})
    target_compile_options(zc_coroutine PRIVATE ${my_compile_definitions})
    zcc_problem_FILE_macro(zc_coroutine)
    set_target_properties(zc_coroutine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)
ELSE()
    ADD_LIBRARY(zc_coroutine STATIC src/stdlib/log.c)
ENDIF(CMAKE_SYSTEM_NAME MATCHES "Linux")

file(GLOB SAMPLE_SOURCES "sample/*/CMakeLists.txt" "cpp_sample/*/CMakeLists.txt")

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
ELSE()
    FILE(GLOB TMP_SOURCES "sample/event/CMakeLists.txt" "cpp_sample/event/CMakeLists.txt" "sample/master/CMakeLists.txt" "cpp_sample/master/CMakeLists.txt" "sample/coroutine/CMakeLists.txt")
    list(REMOVE_ITEM SAMPLE_SOURCES ${TMP_SOURCES})
ENDIF()

foreach(file ${SAMPLE_SOURCES})
    string(REGEX REPLACE "(.*)/[^/]*" "\\1" path ${file})
    math(EXPR build_target_auto_int_pname_idx "${build_target_auto_int_pname_idx} + 1")
    add_subdirectory(${path})
endforeach()
