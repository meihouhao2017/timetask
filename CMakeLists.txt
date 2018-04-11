cmake_minimum_required(VERSION 2.6)

project (TimeTaskDemo)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include_directories(./include)

aux_source_directory(./src/ DES_SRCS)
aux_source_directory(./test/ DEMO_SRCS)

MESSAGE(STATUS "This is SOURCE dir "${DIR_SRCS})

list(APPEND DIR_SRCS)

MESSAGE(STATUS "This is SOURCE dir "${DIR_SRCS})

add_library(timetask SHARED ${DES_SRCS})
add_library(timetask_static STATIC ${DES_SRCS})

set_target_properties(timetask
        PROPERTIES CLEAN_DIRECT_OUTPUT 1
        VERSION         0.8
        SOVERSION       0
        )
set_target_properties(timetask_static
        PROPERTIES CLEAN_DIRECT_OUTPUT 1
        OUTPUT_NAME "timetask"
        )
target_link_libraries(timetask "libpthread.so")

add_executable(test ${DEMO_SRCS}) 
target_link_libraries(test "timetask")
link_directories("/usr/local/timetask/lib/libtimetask.so" "/usr/lib/")

install(TARGETS timetask timetask_static
        LIBRARY DESTINATION timetask/lib
        ARCHIVE DESTINATION timetask/lib
        )

install(DIRECTORY ./include DESTINATION timetask
        FILES_MATCHING PATTERN "*.h"
        PATTERN ".git*" EXCLUDE
        )
