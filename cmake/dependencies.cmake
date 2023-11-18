include(ExternalProject)

if (CLYPSALOT_BUILD_BOOST)
    include(${CLYPSALOT_CMAKE_DIR}/boost.cmake)
else()
    find_package(Boost REQUIRED OPTIONAL_COMPONENTS unit_test_framework)
endif()

find_package(Doxygen)
find_package(Qt6 COMPONENTS Widgets)
find_package(Threads REQUIRED)
