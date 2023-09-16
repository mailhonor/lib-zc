
set(sample_my_libs )
set(sometarget_my_libs )
set(list_my_libs ssl crypto)
set(append_my_libs ssl crypto)

IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(sample_my_libs iconv pthread)
ENDIF()

IF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(sample_my_libs iconv pthread)
ENDIF()

include(../../dev/sample.cmake)
