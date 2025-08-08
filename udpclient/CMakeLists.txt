set(SOURCES
    main.cpp
    udpclient.h
    udpclient.cpp
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/client.conf
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_executable(udpclient ${SOURCES})

target_link_libraries(udpclient vmscore)
