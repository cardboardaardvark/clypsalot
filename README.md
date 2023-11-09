Clypsalot Dataflow Engine and Graphical User Interface

This project is an abstract data filtering system intended especially for the domain of audio
effects procesing. The concept is basically [GNU Radio](https://en.wikipedia.org/wiki/GNU_Radio)
but for audio equalizers, noise gates and dynamic range compressors. However the core engine is
not limited to just PCM audio data.

Clypsalot is currently in development with major changes occuring constantly. Presently some basic
GUI functionality exists and non-audio test objects are able to run.

# Building

The project is written in C++20 and uses the CMake build system. Development is occuring
under Debian Bookworm GNU/Linux using Clang version 17. Project goals include being compatible
with the GCC and Microsoft Visual C++ compilers as well as additionally targetting at least the
FreeBSD and Windows operating systems.

The following are hard dependencies for compiling and the Debian packages that provide them

* Boost Core, libboost-dev

The following optional dependencies will be used if present

* Boost Test, libboost-dev
* Doxygen, doxygen
* Qt 6, qt6-base-dev

By default the Clypsalot library will be built as well as the GUI if Qt is present. The following
is an example of how to do so from the root directory of the project:

    mkdir build
    cd build
    cmake ..
    cmake --build .

To build and run the software tests:

    cmake --build . -t validate

To generate the documentation:

    cmake --build . -t doc
