file (GLOB LIBBOXYSEQ_SOURCES *.c)


include_directories(${GLIB2_INCLUDE_DIRS})


add_library( boxyseq ${LIBBOXYSEQ_SOURCES})

