
# set(general_coroutine_server_httpd_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a ssl crypto pthread)


IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(coroutine_httpd_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a ssl crypto)
	set(explore_httpd_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a)
	set(general_coroutine_server_httpd_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a ssl crypto)
	set(websocketd_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a ssl crypto)
	set(sample_my_libs pthread)
	set(simple_httpd_my_libs ssl crypto)
ENDIF()

IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(sample_my_libs iconv pthread)
	set(simple_httpd_my_libs ssl crypto)
ENDIF()

include(../../dev/sample.cmake)

