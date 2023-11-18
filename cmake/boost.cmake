set(BOOST_VERSION 1.83.0)
set(BOOST_SHA256 c86bd9d9eef795b4b0d3802279419fde5221922805b073b9bd822edecb1ca28e)

string(REPLACE "." "_" BOOST_DOWNLOAD_PREFIX ${BOOST_VERSION})
set(BOOST_EXTENSION zip)
set(BOOST_FILENAME ${BOOST_VERSION}.${BOOST_EXTENSION})
set(BOOST_URL https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_DOWNLOAD_PREFIX}.${BOOST_EXTENSION})
set(BOOST_PREFIX ${CMAKE_BINARY_DIR}/boost)

if (WIN32)
    set(BOOST_BOOTSTRAP bootstrap.bat)
else()
    set(BOOST_BOOTSTRAP bootstrap.sh)
endif()

ExternalProject_Add(
    boost

    URL ${BOOST_URL}
    URL_HASH SHA256=${BOOST_SHA256}

    BUILD_IN_SOURCE yes
    CONFIGURE_COMMAND ${BOOST_BOOTSTRAP}
    BUILD_COMMAND b2 install -j${NUM_CPU} --prefix=${BOOST_PREFIX} --with-test --link=dynamic --variant=release --threading=muti
    INSTALL_COMMAND ""
)

set(Boost_INCLUDE_DIRS ${BOOST_PREFIX}/include)
set(Boost_LIBRARY_DIRS ${BOOST_PREFIX}/lib)
set(Boost_LIBRARIES boost_unit_test_framework)
set(Boost_UNIT_TEST_FRAMEWORK_FOUND yes)
