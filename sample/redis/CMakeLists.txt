
set(sample_my_libs )
set(sometarget_my_libs )

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(puny_server_my_libs ${CMAKE_CURRENT_SOURCE_DIR}/../../libzc_coroutine.a pthread)
ENDIF()

include(../../dev/sample.cmake)
