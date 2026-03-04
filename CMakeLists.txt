cmake_minimum_required(VERSION 3.10)

project(lib-zc)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 通用工具
include(./dev/cmake_general_utils.cmake)

# 其他个性化设置
if(EXISTS "${PROJECT_SOURCE_DIR}/dev/extra.cmake")
    include("${PROJECT_SOURCE_DIR}/dev/extra.cmake")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

if(ZCC_LINUX)
    set(CMAKE_EXE_LINKER_FLAGS -rdynamic)
endif()
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Default build type: Release" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")

include_directories(
    ./
    ./include/
    ${general_include_dirs}
)

link_directories(
    ./
    ${general_lib_dirs}
)

find_path(uconv_include_dir NAME unicode/ucnv.h
    HINTS ${general_include_dirs}
    PATH_SUFFIXES /include/
)

find_path(sqlite3_include_dir NAME sqlite3.h
    HINTS ${general_include_dirs}
    PATH_SUFFIXES /include/
)

# C 基础库, 协程库
if(ZCC_LINUX)
    file(GLOB COROUTINE_SOURCES "src/coroutine/*.c")
    file(GLOB ALL_C_SOURCES "src/*/*.c")
    list(REMOVE_ITEM ALL_C_SOURCES ${COROUTINE_SOURCES})
    #
    add_library(zc_static STATIC ${ALL_C_SOURCES})
    zcc_problem_FILE_macro(zc_static)
    set_target_properties(zc_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)
    #
    add_library(zc_coroutine_static STATIC ${COROUTINE_SOURCES})
    zcc_problem_FILE_macro(zc_coroutine_static)
    set_target_properties(zc_coroutine_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)
    #
    add_library(zc_coroutine SHARED ${COROUTINE_SOURCES})
    zcc_problem_FILE_macro(zc_coroutine)
    set_target_properties(zc_coroutine PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)
endif(ZCC_LINUX)

# zcc sources
file(GLOB ALL_CPP_STATIC_SOURCES "cpp_src/*/*.cpp")
file(GLOB ALL_CPP_SHARED_SOURCES "cpp_src/*/*.cpp")
if(ZCC_LINUX)
else()
    file(GLOB TMP_SOURCES "cpp_src/event/*.cpp cpp_src/server/*.cpp")
    list(REMOVE_ITEM ALL_CPP_STATIC_SOURCES ${TMP_SOURCES})
    list(REMOVE_ITEM ALL_CPP_SHARED_SOURCES ${TMP_SOURCES})
endif()

# icu
file(GLOB ALL_CPP_ICU_SOURCES "cpp_src/charset/uconv.cpp")
list(REMOVE_ITEM ALL_CPP_SHARED_SOURCES ${ALL_CPP_ICU_SOURCES})
if(uconv_include_dir)
    message("found libicu")
    add_definitions(-DZCC_USE_UCONV__)
    set(general_libs icuuc icudata icui18n ${general_libs})
    add_library(zcc_icu SHARED ${ALL_CPP_ICU_SOURCES})
    zcc_problem_FILE_macro(zcc_icu)
    set_target_properties(zcc_icu PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/ LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/ LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/)
    set_target_properties(zcc_icu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/ RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/ RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/)
    target_compile_definitions(zcc_icu PRIVATE ZCC_DEV_MODE___)
    add_dependencies(zcc_icu zcc)
    if(ZCC_WINDOWS)
        target_link_libraries(zcc_icu zcc ${general_libs})
    endif()
else()
    message("WARN: not found libicu")
endif()

# sqlite3
file(GLOB ALL_CPP_SQLITE3_SOURCES "cpp_src/sqlite3/*.cpp")
list(REMOVE_ITEM ALL_CPP_SHARED_SOURCES ${ALL_CPP_SQLITE3_SOURCES})
if(sqlite3_include_dir)
    message("found sqlite3")
    add_definitions(-DZCC_USE_SQLITE3__)
    set(general_libs sqlite3 ${general_libs})
    add_library(zcc_sqlite3 SHARED ${ALL_CPP_SQLITE3_SOURCES})
    zcc_problem_FILE_macro(zcc_sqlite3)
    set_target_properties(zcc_sqlite3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/ LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/ LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/)
    set_target_properties(zcc_sqlite3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/ RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/ RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/)
    target_compile_definitions(zcc_sqlite3 PRIVATE ZCC_DEV_MODE___)
    add_dependencies(zcc_sqlite3 zcc)
    if(ZCC_WINDOWS)
        target_link_libraries(zcc_sqlite3 zcc ${general_libs})
    endif()
else()
    message("WARN: not found sqlite3")
endif()

# zcc coroutine extends
file(GLOB ALL_CPP_COROUTINE_SOURCES "cpp_src/server/coroutine_worker_server.cpp")
list(REMOVE_ITEM ALL_CPP_SHARED_SOURCES ${ALL_CPP_COROUTINE_SOURCES})
if(ZCC_LINUX)
    add_library(zcc_coroutine_extend SHARED ${ALL_CPP_COROUTINE_SOURCES})
    zcc_problem_FILE_macro(zcc_coroutine_extend)
    set_target_properties(zcc_coroutine_extend PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)
    target_compile_definitions(zcc_coroutine_extend PRIVATE ZCC_DEV_MODE___)
    add_dependencies(zcc_coroutine_extend zcc zc_coroutine)
endif()

# zcc static
add_library(zcc_static STATIC ${ALL_CPP_STATIC_SOURCES})
zcc_problem_FILE_macro(zcc_static)
set_target_properties(zcc_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/ ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/" ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/")
target_compile_definitions(zcc_static PRIVATE ZCC_DEV_MODE___)

# zcc shared
add_library(zcc SHARED ${ALL_CPP_SHARED_SOURCES})
zcc_problem_FILE_macro(zcc)
set_target_properties(zcc PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/ LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/ LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/)
set_target_properties(zcc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/ RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/ RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/)
target_compile_definitions(zcc PRIVATE ZCC_DEV_MODE___)
if(ZCC_WINDOWS)
    target_link_libraries(zcc ${general_libs})
endif()

# sample sources
if(ZCC_LINUX)
    file(GLOB SAMPLE_SOURCES "coroutine_sample/*.c" "sample/*/*.c" "cpp_sample/*/*.cpp")
else()
    file(GLOB SAMPLE_SOURCES "cpp_sample/*/*.cpp")
    file(GLOB TMP_SOURCES "cpp_sample/event/*.cpp" "cpp_sample/server/*.cpp" "cpp_sample/*/*coroutine*.cpp")
    list(REMOVE_ITEM SAMPLE_SOURCES ${TMP_SOURCES})
endif()

# all samples
if(ZCC_WINDOWS)
    message("should define ZCC_DLL_MODE when use dll")
endif()
set(name_idx 100)
foreach(file ${SAMPLE_SOURCES})
    math(EXPR name_idx "${name_idx} + 1")
    string(REGEX REPLACE ".*/(.*)" "\\1" bin ${file})
    string(REGEX REPLACE "\\.cpp$" "" bin ${bin})
    string(REGEX REPLACE "\\.c$" "" bin ${bin})

    string(REGEX REPLACE "(.*)/.*" "\\1" path ${file})
    string(REGEX REPLACE ".*/([^//]*)" "\\1" parent_dir ${path})
    string(REGEX REPLACE ".*/([^//]*)/[^//]*" "\\1" pparent_dir ${path})

    set(tmp_target ${name_idx}-${bin})

    if(${pparent_dir} STREQUAL "cpp_sample")
        set(tmp_target cpp-${parent_dir}-${bin})
    endif()

    if(${pparent_dir} STREQUAL "sample")
        set(tmp_target c-${parent_dir}-${bin})
    endif()

    if(${pparent_dir} STREQUAL "coroutine_sample")
        set(tmp_target co-${bin})
    endif()

    add_executable(${tmp_target} ${file})

    set(mylibs "")

    if(${file} MATCHES "coroutine|puny_server.c|websocketd.c|explore_httpd.c")
        set(mylibs ${mylibs} zc_coroutine_static)
    endif()

    set_target_properties(${tmp_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/${pparent_dir}/${parent_dir} RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/${pparent_dir}/${parent_dir} RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/${pparent_dir}/${parent_dir})
    set_target_properties(${tmp_target} PROPERTIES RUNTIME_OUTPUT_NAME ${bin})
    set_property(TARGET ${tmp_target} PROPERTY FOLDER ${pparent_dir}/${parent_dir})

    if(${pparent_dir} STREQUAL "cpp_sample")
        target_link_libraries(${tmp_target} ${mylibs} zcc_static ${general_libs})
        add_dependencies(${tmp_target} zcc_static)
    endif()

    if(${pparent_dir} STREQUAL "sample")
        target_link_libraries(${tmp_target} zc_static ${mylibs} ${general_libs})
        add_dependencies(${tmp_target} zc_static)
    endif()

    if(ZCC_LINUX)
        add_dependencies(${tmp_target} zc_coroutine_static)
    endif(ZCC_LINUX)

    # if(ZCC_WINDOWS)
    #     set_target_properties(${tmp_target} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
    # endif(ZCC_WINDOWS)

    zcc_problem_FILE_macro(${tmp_target})
endforeach()
