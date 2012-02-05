cmake_minimum_required(VERSION 2.6)

if (COMMAND CMAKE_POLICY)
  cmake_policy (SET CMP0011 NEW)
endif (COMMAND CMAKE_POLICY)


project(BoxySeq)


set(BoxySeq_VERSION_MAJOR 0)
set(BoxySeq_VERSION_MINOR 0)


mark_as_advanced(EXECUTABLE_OUTPUT_PATH)
mark_as_advanced(LIBRARY_OUTPUT_PATH)
mark_as_advanced(CMAKE_BUILD_TYPE)
mark_as_advanced(CMAKE_INSTALL_PREFIX)

option (BuildForDebug "Include gdb debugging support" ON)
set (BuildOptionsDebug "-O0 -ggdb" CACHE STRING "Debug build flags")

# additional GCC flags
add_definitions(-Wall -Wextra -DSRC_DIR=\"${CMAKE_SOURCE_DIR}\")

if (BuildForDebug)
    set (CMAKE_BUILD_TYPE "Debug")
    set (DEBUG 1)
    set (CMAKE_C_FLAGS_DEBUG ${BuildOptionsDebug})
    message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_C_FLAGS_DEBUG}")
else (BuildForDebug)
    set (CMAKE_BUILD_TYPE "Release")
    set (DEBUG 0)
    set (CMAKE_C_FLAGS_RELEASE ${BuildOptionsBasic})
    message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_C_FLAGS_RELEASE}")
endif (BuildForDebug)

# PKG-CONFIG
find_package(PkgConfig REQUIRED)
if(PKG_CONFIG_FOUND)
    message(STATUS "Found pkg-config ${PKG_CONFIG_EXECUTABLE}")
else(PKG_CONFIG_FOUND)
    message(FATAL_ERROR "pkg-config required but not found")
endif(PKG_CONFIG_FOUND)


pkg_check_modules(JACK REQUIRED jack>=0.118.0)
if (JACK_FOUND)
  message(STATUS "Found jack ${JACK_VERSION}")
else (JACK_FOUND)
  message(FATAL_ERROR "Jack >= 0.118.0 not found.")
endif (JACK_FOUND)


ADD_SUBDIRECTORY( libboxyseq )
ADD_SUBDIRECTORY( boxyseq_gui )
ADD_SUBDIRECTORY( freespace_test )
