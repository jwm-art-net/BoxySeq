file (GLOB LIBBOXYSEQ_SOURCES *.c)


include_directories(${GLIB2_INCLUDE_DIRS})


add_library( boxyseq ${LIBBOXYSEQ_SOURCES})
add_definitions(-DUSE_8BIT_ARRAY)

target_link_libraries( boxyseq m)
