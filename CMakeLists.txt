cmake_minimum_required(VERSION 2.6)

if (COMMAND CMAKE_POLICY)
  cmake_policy (SET CMP0011 NEW)
endif (COMMAND CMAKE_POLICY)


project(BoxySeq)


set(BoxySeq_VERSION_MAJOR 0)
set(BoxySeq_VERSION_MINOR 0)


set(DEFAULT_GUI "gdk" CACHE STRING "Default GUI - gdk, cairo, or fltk")


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


pkg_check_modules (GLIB2 REQUIRED glib-2.0>=2.16)
if (GLIB2_FOUND)
  message(STATUS "Found glib2 ${GLIB2_VERSION}")
else (GLIB2_FOUND)
  message(FATAL_ERROR "glib2 was not found.")
endif (GLIB2_FOUND)


ADD_SUBDIRECTORY( libboxyseq )

if (DEFAULT_GUI STREQUAL "gdk")
  ADD_SUBDIRECTORY( boxyseq_gtk )
elseif (DEFAULT_GUI STREQUAL "cairo")
  ADD_SUBDIRECTORY( boxyseq_gtk_cairo )
elseif (DEFAULT_GUI STREQUAL "fltk")
  ADD_SUBDIRECTORY( boxyseq_fltk )
endif (DEFAULT_GUI STREQUAL "gdk")
