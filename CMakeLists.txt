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

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Werror -Wfatal-errors)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR})

# The trailing directory seperator is intentionally included
add_compile_definitions(CLYPSALOT_PROJECT_DIR="${CMAKE_SOURCE_DIR}/")

add_subdirectory(${CLYPSALOT_LIBRARY_SOURCE_DIRECTORY})

add_executable(test test.cxx)
target_link_libraries(test clypsalot)

find_package(Doxygen)

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
