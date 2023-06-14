
set(sample_my_libs )
set(sometarget_my_libs )

IF (ENABLE_ICU_UCONV)
    set(sample_my_libs icuuc icudata icui18n pthread)
ELSE ()
	IF(CMAKE_SYSTEM_NAME MATCHES "Windows")
		set(sample_my_libs iconv pthread)
	ENDIF()
ENDIF()

include(../../dev/sample.cmake)
