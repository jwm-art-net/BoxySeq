include_directories(${BoxySeq_SOURCE_DIR}/libboxyseq)

find_package(GTK2 2.12 REQUIRED gtk)

if (GTK2_FOUND)
  message(STATUS "Found gtk2 ${GTK2_VERSION}")
else (GTK2_FOUND)
  message(FATAL_ERROR "gtk2 was not found.")
endif (GTK2_FOUND)


include_directories(${GTK2_INCLUDE_DIRS})

file (GLOB BOXYSEQ_GUI_SOURCES *.c)

add_executable(boxyseq_gui ${BOXYSEQ_GUI_SOURCES})

target_link_libraries(boxyseq_gui boxyseq ${GTK2_LIBRARIES})
