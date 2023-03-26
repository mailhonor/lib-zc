
# set(general_coroutine_server_httpd_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a ssl crypto pthread)

set(sample_my_libs pthread)
set(coroutine_httpd_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a ssl crypto)
set(explore_httpd_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a)
set(general_coroutine_server_httpd_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a ssl crypto)
set(simple_httpd_my_libs ssl crypto)

include(../../dev/sample.cmake)

