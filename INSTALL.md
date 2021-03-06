Installing a binary distribution package
========================================

No official binary distribution packages are provided by the jflags developers.
There may, however, be binary packages available for your OS. Please consult
also the package repositories of your Linux distribution.

For example on Debian/Ubuntu Linux, jflags can be installed using the
following command:

    sudo apt-get install libjflags-dev


Compiling the source code with CMake
=========================

The build system of jflags is since version 2.1 based on [CMake](http://cmake.org).
The common steps to build, test, and install software are therefore:

1. Extract source files.
2. Create build directory and change to it.
3. Run CMake to configure the build tree.
4. Build the software using selected build tool.
5. Test the built software.
6. Install the built files.

On Unix-like systems with GNU Make as build tool, these build steps can be
summarized by the following sequence of commands executed in a shell,
where ```$package``` and ```$version``` are shell variables which represent
the name of this package and the obtained version of the software.

    $ tar xzf jflags-$version-source.tar.gz
    $ cd jflags-$version
    $ mkdir build && cd build
    $ ccmake ..
    
      - Press 'c' to configure the build system and 'e' to ignore warnings.
      - Set CMAKE_INSTALL_PREFIX and other CMake variables and options.
      - Continue pressing 'c' until the option 'g' is available.
      - Then press 'g' to generate the configuration files for GNU Make.
    
    $ make
    $ make test    (optional)
    $ make install (optional)

In the following, only jflags-specific CMake settings available to
configure the build and installation are documented. Note that most of these
variables are for advanced users and binary package maintainers only.
They usually do not have to be modified.


CMake Option                | Description
--------------------------- | -------------------------------------------------------
CMAKE_INSTALL_PREFIX        | Installation directory, e.g., "/usr/local" on Unix and "C:\Program Files\jflags" on Windows.
BUILD_SHARED_LIBS           | Request build of dynamic link libraries.
BUILD_STATIC_LIBS           | Request build of static link libraries. Implied if BUILD_SHARED_LIBS is OFF.
BUILD_PACKAGING             | Enable binary package generation using CPack.
BUILD_TESTING               | Build tests for execution by CTest.
BUILD_NC_TESTS              | Request inclusion of negative compilation tests (requires Python).
BUILD_CONFIG_TESTS          | Request inclusion of package configuration tests (requires Python).
BUILD_jflags_LIBS           | Request build of multi-threaded jflags libraries (if threading library found).
BUILD_jflags_nothreads_LIBS | Request build of single-threaded jflags libraries.
JFLAGS_NAMESPACE            | Name of the C++ namespace to be used by the jflags library. Note that the public source header files are installed in a subdirectory named after this namespace. To maintain backwards compatibility with the Google Commandline Flags, set this variable to "google". The default is "jflags".
JFLAGS_INTTYPES_FORMAT      | String identifying format of built-in integer types.
JFLAGS_INCLUDE_DIR          | Name of headers installation directory relative to CMAKE_INSTALL_PREFIX.
LIBRARY_INSTALL_DIR         | Name of library installation directory relative to CMAKE_INSTALL_PREFIX.
INSTALL_HEADERS             | Request installation of public header files.

Using jflags with [Bazel](http://bazel.io)
=========================

To use jflags in a Bazel project, map it in as an external dependency by editing
your WORKSPACE file:

    git_repository(
        name = "jflags_git",
        commit = "",  # Use the current HEAD commit
        remote = "https://github.com/jflags/jflags.git",
    )

    bind(
        name = "jflags",
        actual = "@jflags_git//:jflags",
    )

You can then add `//external:jflags` to the `deps` section of a `cc_binary` or
`cc_library` rule, and `#include <jflags/jflags.h>` to include it in your source
code.
