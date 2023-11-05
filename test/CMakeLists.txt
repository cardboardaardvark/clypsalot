unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
unset(CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
unset(CMAKE_LIBRARY_OUTPUT_DIRECTORY)

add_library(
    ${CLYPSALOT_TEST_LIB_TARGET} SHARED EXCLUDE_FROM_ALL

    lib/test.hxx
    lib/test.cxx
    module/module.hxx
    module/module.cxx
    module/object.hxx
    module/object.cxx
    module/port.hxx
    module/port.cxx
)

target_link_libraries(${CLYPSALOT_TEST_LIB_TARGET} PUBLIC clypsalot Boost::unit_test_framework)

add_clypsalot_test(unit test)
add_clypsalot_test(unit util)
add_clypsalot_test(unit logging)
add_clypsalot_test(unit thread)
add_clypsalot_test(unit message)
add_clypsalot_test(unit object)
add_clypsalot_test(unit port)

add_clypsalot_test(integration object)
