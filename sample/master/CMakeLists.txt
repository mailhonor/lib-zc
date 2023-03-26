
set(sample_my_libs )
set(sometarget_my_libs )
set(coroutine_server_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a pthread)
set(master_server_my_libs pthread)

include(../../dev/sample.cmake)

