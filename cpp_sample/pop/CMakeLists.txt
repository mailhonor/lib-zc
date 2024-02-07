
set(sample_my_libs )
set(sometarget_my_libs )
set(pop_client_my_libs ssl crypto)

IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(sample_my_libs pthread)
ENDIF()

IF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(sample_my_libs pthread)
ENDIF()

include(../../dev/sample.cmake)
