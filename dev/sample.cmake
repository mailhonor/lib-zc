option(ENABLE_ICU_UCONV "icu charset convert" OFF)
project(sample_${parent_dir})
string(REGEX REPLACE ".*/([^//]*)" "\\1" parent_dir ${PROJECT_SOURCE_DIR})
string(REGEX REPLACE ".*/([^//]*)/[^//]*" "\\1" pparent_dir ${PROJECT_SOURCE_DIR})
project(${pparent_dir}/${parent_dir})

project(lib-zc)

set(my_compile_definitions
    "-D_GNU_SOURCE"
    "-g" "-ggdb"
    "-O2"
    "-Wall"
    "-Winline"
    "-Wcast-align"
    "-Wno-long-long"
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

set(CMAKE_C_FLAGS "-std=gnu99")
set(CMAKE_CXX_FLAGS "-std=gnu++11")
add_compile_options(${my_compile_definitions})

INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/../../
)

LINK_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/../../
    ${platform_lib_dirs}
)

set(libzc ${PROJECT_SOURCE_DIR}/../../libzc.a)

set(ALL_SAMPLE_TARGETS "")
file(GLOB SAMPLE_SOURCES "*.c" "*.cpp")

foreach(file ${SAMPLE_SOURCES})
    string(REGEX REPLACE ".*/(.*)" "\\1" bin ${file})
    string(REGEX REPLACE ".cpp$" "" bin ${bin})
    string(REGEX REPLACE ".c$" "" bin ${bin})
    set(tmp_target ${bin}--${parent_dir}-${pparent_dir})
    build_target_auto_int_name()
    set(tmp_target ${target_auto_int_name}-${bin})
    add_executable(${tmp_target} ${file})
    target_link_libraries(${tmp_target} ${libzc} ${${bin}_my_libs} ${sample_my_libs} ${platform_libs})
    set_target_properties(${tmp_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/)
    set_target_properties(${tmp_target} PROPERTIES RUNTIME_OUTPUT_NAME ${bin})
    set(ALL_SAMPLE_TARGETS ${tmp_target} ${ALL_SAMPLE_TARGETS})
    add_dependencies(${tmp_target} zc)

    IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
        add_dependencies(${tmp_target} zc_coroutine)
    ENDIF(CMAKE_SYSTEM_NAME MATCHES "Linux")

    zcc_problem_FILE_macro(${tmp_target})
endforeach()

set(tmp_target ALL--${parent_dir}-${pparent_dir})
build_target_auto_int_name()
set(tmp_target ${target_auto_int_name}-ALL)
add_custom_target(${tmp_target} COMMAND)
add_dependencies(${tmp_target} ${ALL_SAMPLE_TARGETS})
