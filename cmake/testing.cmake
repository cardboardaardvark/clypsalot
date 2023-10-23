# This has to be before the include(CTest) and removing CMakeCache.txt will not cause
# CTest to update the options.
# https://stackoverflow.com/questions/52730994/how-to-pass-arguments-to-memcheck-with-ctest
set(MEMORYCHECK_COMMAND_OPTIONS "--error-exitcode=1 --leak-check=full --show-leak-kinds=definite,possible,indirect --show-reachable=no --show-error-list=yes")

include(CTest)

set(CLYPSALOT_TEST_BIN_TARGET tests)
set(CLYPSALOT_VALIDATION_TARGET validate)
set(CLYPSALOT_MEM_VALIDATION_TARGET memcheck)

# Target to build all test programs
add_custom_target(${CLYPSALOT_TEST_BIN_TARGET})
# Target to build all the tests and run testing because the cmake test target
# isn't real
add_custom_target(${CLYPSALOT_VALIDATION_TARGET} ctest)
# Target to run the tests under valgrind/other memory usage validator
add_custom_target(${CLYPSALOT_MEM_VALIDATION_TARGET} ctest -T memcheck)

add_dependencies(${CLYPSALOT_VALIDATION_TARGET} ${CLYPSALOT_TEST_BIN_TARGET})
add_dependencies(${CLYPSALOT_MEM_VALIDATION_TARGET} ${CLYPSALOT_TEST_BIN_TARGET})

function(add_clypsalot_test type name)
    set(TEST_NAME ${type}-${name})
    set(TEST_TARGET test-${TEST_NAME})
    set(TEST_TYPE_TARGET ${type}-tests)

    if(NOT TARGET ${TEST_TYPE_TARGET})
        add_custom_target(${TEST_TYPE_TARGET})
        add_dependencies(${CLYPSALOT_TEST_BIN_TARGET} ${TEST_TYPE_TARGET})
    endif()

    add_executable(${TEST_TARGET} ${type}/${name}.cxx)
    target_compile_definitions(${TEST_TARGET} PRIVATE TEST_NAME="${TEST_NAME}")
    set_target_properties(${TEST_TARGET} PROPERTIES OUTPUT_NAME "${TEST_NAME}")
    target_link_libraries(${TEST_TARGET} ${CLYPSALOT_TEST_LIB_TARGET})
    add_dependencies(${TEST_TYPE_TARGET} ${TEST_TARGET})
    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_NAME})
endfunction()
