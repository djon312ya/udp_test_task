set(SOURCES
    main.cpp
    udpserver.h
    udpserver.cpp
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/server.conf
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_executable(udpserver ${SOURCES})

target_link_libraries(udpserver vmscore)
