
IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(sample_my_libs)
ENDIF()
IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(sample_my_libs iconv)
ENDIF()
set(sometarget_my_libs )

include(../../dev/sample.cmake)

