include_directories(${BoxySeq_SOURCE_DIR}/libboxyseq)

file (GLOB FREESPACE_TEST_SOURCES *.c)

add_executable(freespace_test ${FREESPACE_TEST_SOURCES})

target_link_libraries(freespace_test boxyseq )
