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
    set(TEST_NAME test-${type}-${name})
    set(TEST_TYPE_TARGET ${type}-tests)

    if(NOT TARGET ${TEST_TYPE_TARGET})
        add_custom_target(${TEST_TYPE_TARGET})
        add_dependencies(${CLYPSALOT_TEST_BIN_TARGET} ${TEST_TYPE_TARGET})
    endif()

    add_executable(${TEST_NAME} ${type}/${name}.cxx)
    target_compile_definitions(${TEST_NAME} PRIVATE TEST_NAME="${name}")
    target_link_libraries(${TEST_NAME} ${CLYPSALOT_TEST_LIB_TARGET})
    add_dependencies(${TEST_TYPE_TARGET} ${TEST_NAME})
    add_test(${TEST_NAME} ${TEST_NAME})
endfunction()
