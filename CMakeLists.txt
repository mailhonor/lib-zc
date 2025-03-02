cmake_minimum_required(VERSION 3.10)

project(lib-zc)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# 通用工具
include(./dev/cmake_general_utils.cmake)

IF(EXISTS "${PROJECT_SOURCE_DIR}/dev/general.cmake")
    include("${PROJECT_SOURCE_DIR}/dev/general.cmake")
ENDIF()

find_path(sqlite3_include_dir NAME sqlite3.h
    HINTS ${general_include_dirs}
    PATH_SUFFIXES /include/
)

IF(sqlite3_include_dir)
    option(ENABLE_SQLITE "sqlite3 proxy server" ON)
    message("found sqlite3")
ENDIF()

find_path(uconv_include_dir NAME unicode/ucnv.h
    HINTS ${general_include_dirs}
    PATH_SUFFIXES /include/
)

IF(uconv_include_dir)
    option(ENABLE_ICU_UCONV "icu charset convert" ON)
    message("found libicu")
ENDIF()

# 其他个性化设置
IF(EXISTS "${PROJECT_SOURCE_DIR}/dev/extra.cmake")
    include("${PROJECT_SOURCE_DIR}/dev/extra.cmake")
ENDIF()

add_definitions(-DZC_DEV_MODE___)
add_definitions(-DZCC_DEV_MODE___)

IF(ENABLE_ICU_UCONV)
    add_definitions(-DZCC_USE_UCONV__)
    set(general_libs icuuc icudata icui18n ${general_libs})
ENDIF()

IF(ENABLE_SQLITE)
    add_definitions(-DZCC_USE_SQLITE3__)
    set(general_libs sqlite3 ${general_libs})
ENDIF()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

FILE(GLOB COROUTINE_SOURCES "src/coroutine/*.c")
FILE(GLOB ALL_C_SOURCES "src/*/*.c")
list(REMOVE_ITEM ALL_C_SOURCES ${COROUTINE_SOURCES})
FILE(GLOB ALL_CPP_SOURCES "cpp_src/*/*.cpp")

IF(ZCC_LINUX)
ELSE()
    FILE(GLOB TMP_SOURCES "cpp_sample/event/*.cpp cpp_sample/server/*.cpp")
    list(REMOVE_ITEM ALL_CPP_SOURCES ${TMP_SOURCES})
ENDIF()

IF(ZCC_LINUX)
    set(CMAKE_EXE_LINKER_FLAGS -rdynamic)
ENDIF()

INCLUDE_DIRECTORIES(
    ./
    ./include/
    ${general_include_dirs}
)

LINK_DIRECTORIES(
    ./
    ${general_lib_dirs}
)

IF(ZCC_LINUX)
    ADD_LIBRARY(zc STATIC ${ALL_C_SOURCES})
    zcc_problem_FILE_macro(zc)
    set_target_properties(zc PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)

    ADD_LIBRARY(zc_coroutine STATIC ${COROUTINE_SOURCES})
    zcc_problem_FILE_macro(zc_coroutine)
    set_target_properties(zc_coroutine PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)
ENDIF(ZCC_LINUX)

ADD_LIBRARY(zcc STATIC ${ALL_CPP_SOURCES})
zcc_problem_FILE_macro(zcc)
set_target_properties(zcc PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)

IF(MSVC)
    file(GLOB SAMPLE_SOURCES "cpp_sample/*/*.cpp")
ELSEIF(ZCC_LINUX)
    file(GLOB SAMPLE_SOURCES "coroutine_sample/*.c" "sample/*/*.c" "cpp_sample/*/*.cpp")
ELSE()
    file(GLOB SAMPLE_SOURCES "cpp_sample/*/*.cpp")
ENDIF(MSVC)

IF(ZCC_LINUX)
ELSE()
    FILE(GLOB TMP_SOURCES "sample/event/*.c" "cpp_sample/event/*.cpp" "sample/server/*.c" "cpp_sample/server/*.cpp" "sample/coroutine/*.c")
    list(REMOVE_ITEM SAMPLE_SOURCES ${TMP_SOURCES})
ENDIF()

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

    IF(${pparent_dir} STREQUAL "cpp_sample")
        set(tmp_target cpp-${parent_dir}-${bin})
    ENDIF()

    IF(${pparent_dir} STREQUAL "sample")
        set(tmp_target c-${parent_dir}-${bin})
    ENDIF()

    IF(${pparent_dir} STREQUAL "coroutine_sample")
        set(tmp_target co-${bin})
    ENDIF()

    add_executable(${tmp_target} ${file})
    set(mylibs "")

    IF(ZCC_LINUX)
        IF(${file} MATCHES "coroutine|puny_server.c|websocketd.c|explore_httpd.c")
            set(mylibs $<TARGET_FILE:zc_coroutine>)
        ENDIF()
    ENDIF()

    set_target_properties(${tmp_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/${pparent_dir}/${parent_dir})
    set_target_properties(${tmp_target} PROPERTIES RUNTIME_OUTPUT_NAME ${bin})
    set_property(TARGET ${tmp_target} PROPERTY FOLDER ${pparent_dir}/${parent_dir})

    IF(${pparent_dir} STREQUAL "cpp_sample")
        target_link_libraries(${tmp_target} $<TARGET_FILE:zcc> ${mylibs} ${general_libs})
        add_dependencies(${tmp_target} zcc)
    ENDIF()

    IF(${pparent_dir} STREQUAL "sample")
        IF(ZCC_LINUX)
            target_link_libraries(${tmp_target} $<TARGET_FILE:zc> ${mylibs} ${general_libs})
            add_dependencies(${tmp_target} zc)
        ENDIF(ZCC_LINUX)
    ENDIF()

    IF(ZCC_LINUX)
        add_dependencies(${tmp_target} zc_coroutine)
    ENDIF(ZCC_LINUX)

    zcc_problem_FILE_macro(${tmp_target})
endforeach()
