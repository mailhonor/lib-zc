
set(sample_my_libs )
set(sometarget_my_libs )
if (ENABLE_SQLITE)
    set(sample_my_libs sqlite3 pthread)
endif()

include(../../dev/sample.cmake)
