
set(sample_my_libs )
set(sometarget_my_libs )

if (ENABLE_ICU_UCONV)
    set(sample_my_libs icuuc icudata icui18n pthread)
endif()

include(../../dev/sample.cmake)
