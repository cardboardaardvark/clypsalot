# The version that comes with Debian Bookworm - it is likely set too high for
# the features actually used
cmake_minimum_required(VERSION 3.25.1)

project(clypsalot VERSION 0.0.0 LANGUAGES CXX)

set(CLYPSALOT_BUILD_TYPE Debug)
set(CLYPSALOT_BIN_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CLYPSALOT_LIB_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CLYPSALOT_DOC_DIRECTORY ${CMAKE_BINARY_DIR}/doc)

set(CMAKE_BUILD_TYPE ${CLYPSALOT_BUILD_TYPE})
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CLYPSALOT_BIN_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CLYPSALOT_LIB_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CLYPSALOT_LIB_DIRECTORY})

set(CLYPSALOT_LIBRARY_SOURCE_DIRECTORY ${CMAKE_SOURCE_DIR}/clypsalot)
set(CLYPSALOT_CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)
set(CLYPSALOT_LIB_TARGET clypsalot)
set(CLYPSALOT_TEST_LIB_TARGET clypsalot-test)

include(${CLYPSALOT_CMAKE_DIR}/dependencies.cmake)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Werror -Wfatal-errors)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Valgrind does not understand the debug info from LLVM version 17 until version
    # 3.20 which is not available in Debian yet (not even in backports as I write this).
    add_compile_options(-gdwarf-4)
endif()

# The Clypsalot headers are included as system headers so #include <clypsalot/someheader.hxx> works
include_directories(SYSTEM ${CMAKE_SOURCE_DIR})

# The trailing directory seperator is intentionally included as it is used by the logging system
# to remove the path to the project directory from the filename output.
add_compile_definitions(CLYPSALOT_PROJECT_DIR="${CMAKE_SOURCE_DIR}/")

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

add_subdirectory(${CLYPSALOT_LIBRARY_SOURCE_DIRECTORY})
add_subdirectory(test)

add_executable(test test.cxx)
target_link_libraries(test ${CLYPSALOT_LIB_TARGET} ${CLYPSALOT_TEST_LIB_TARGET})

if(${DOXYGEN_FOUND})
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/doc)
    set(DOXYGEN_QUIET YES)
    set(DOXYGEN_CLASS_GRAPH NO)
    set(DOXYGEN_COLLABORATION_GRAPH NO)
    set(DOXYGEN_INCLUDE_GRAPH NO)
    set(DOXYGEN_INCLUDED_BY_GRAPH NO)
    set(DOXYGEN_SHOW_INCLUDE_FILES NO)
    doxygen_add_docs(doc WORKING_DIRECTORY ${CLYPSALOT_LIBRARY_SOURCE_DIRECTORY})
endif()
