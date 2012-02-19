file (GLOB LIBBOXYSEQ_SOURCES *.c)


include_directories(${GLIB2_INCLUDE_DIRS})


add_library( boxyseq ${LIBBOXYSEQ_SOURCES})
add_definitions(-DUSE_32BIT_ARRAY -DEVPOOL_DEBUG -DEVPOOL_DEBUG999 -DEVPORT_DEBUG)

target_link_libraries( boxyseq m ${JACK_LIBRARIES} ${GLIB_LIBARIES})
