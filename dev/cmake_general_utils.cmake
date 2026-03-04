# 基础环境
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(ZCC_LINUX "Linux")

    set(general_include_dirs)
    set(general_lib_dirs)
    set(general_libs ssl crypto resolv pthread)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(ZCC_WINDOWS "Windows")

    if(MINGW)
        set(general_include_dirs C:/_dev/vcpkg-master/installed/x64-mingw-static/include C:/_dev/mingw64/x86_64-w64-mingw32/include)
        set(general_lib_dirs C:/_dev/vcpkg-master/installed/x64-mingw-static/lib C:/_dev/mingw64/x86_64-w64-mingw32/lib)
        set(general_libs iconv ssl crypto Winmm Ws2_32 Crypt32 Iphlpapi)
    endif(MINGW)

    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set(general_include_dirs C:/_dev/vcpkg-master/installed/x64-windows-static/include)
        set(general_lib_dirs C:/_dev/vcpkg-master/installed/x64-windows-static/lib)
        set(general_libs Winmm Ws2_32 Crypt32 Iphlpapi iconv libssl libcrypto)
    endif(MSVC)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(general_include_dirs /opt/homebrew/Cellar/openssl@3/3.3.1/include/ /usr/local/Cellar/openssl@3/3.3.1/include/)
    set(general_lib_dirs /usr/local/lib/ /opt/homebrew/Cellar/openssl@3/3.3.1/lib /usr/local/Cellar/openssl@3/3.3.1/lib)
    set(general_libs iconv ssl crypto resolv pthread)
endif()

if(MINGW OR ZCC_LINUX OR APPLE)
    add_definitions(
        "-D_GNU_SOURCE"
        "-D_POSIX"
        "-g" "-ggdb"
        "-O2"
        "-Wall"
        "-Wcast-align"
        "-Wno-long-long"
        "-Wno-format-zero-length"
        "-Wpointer-arith"
        "-D_REENTRANT"
        "-D_POSIX_PTHREAD_SEMANTICS"
        "-Wno-unused"
        "-fPIC"
    )
endif()

if(MSVC)
    add_definitions(-utf-8 -EHs /W3)
    # Release模式：/MD
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD /O2")
    # Debug模式：/MDd
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Zi")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd /Zi")
    
    # 禁用CMAKE的默认CRT自动选择（关键）
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

if(ZCC_LINUX)
    set(CMAKE_EXE_LINKER_FLAGS -rdynamic)
endif()

# 解决 __FILE__ 宏绝对路径的问题
function(zcc_problem_FILE_macro targetname)
    get_target_property(source_files "${targetname}" SOURCES)

    foreach(sourcefile ${source_files})
        get_property(defs SOURCE "${sourcefile}" PROPERTY COMPILE_DEFINITIONS)
        get_filename_component(filepath "${sourcefile}" ABSOLUTE)
        string(REPLACE ${PROJECT_SOURCE_DIR}/ "" relpath ${filepath})
        list(APPEND defs "ZCC__FILE__=\"${relpath}\"")
        set_property(
            SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS ${defs}
        )
    endforeach()
endfunction()

# 合并经静态库
function(zcc_combine_static_libs targetname_from_func static_libs_from_func)
    set(static_libs "${ARGV}")
    list(GET static_libs 0 targetname)
    list(REMOVE_AT static_libs 0)

    file(REMOVE ${targetname})

    if(APPLE)
        add_custom_command(OUTPUT ${targetname}
            COMMAND libtool -static -o ${targetname} ${static_libs}
            DEPENDS ${static_libs})
    elseif(ZCC_WINDOWS)
        add_custom_command(OUTPUT ${targetname}
            COMMAND lib.exe /OUT:${targetname} ${static_libs}
            DEPENDS ${static_libs})
    else()
        file(WRITE ${PROJECT_SOURCE_DIR}/build/_tmp_ar_.sh "
echo \"create \$1\" > zcc_1.mri
shift
while [ \"\$1\" != \"\" ]; do
echo \"addlib \$1\" >> zcc_1.mri
shift
done
echo save >> zcc_1.mri
echo end >> zcc_1.mri
ar -M < zcc_1.mri
        ")
        add_custom_command(OUTPUT ${targetname}
            COMMAND sh ${PROJECT_SOURCE_DIR}/build/_tmp_ar_.sh ${targetname} ${static_libs}
            DEPENDS ${static_libs})
    endif()
endfunction()

# windows 
function(zcc_export_all_symbols targetname)
    if(MSVC)
        set_property(TARGET ${targetname} PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS ON)
    elseif(MINGW)
        target_link_options(${targetname} PRIVATE
            -Wl,--export-all-symbols
            -Wl,--enable-auto-import
        )
    endif()
endfunction()