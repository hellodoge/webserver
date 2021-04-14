cmake_minimum_required(VERSION 3.15)
project(webserver C)

set(CMAKE_C_STANDARD 11)

add_executable(webserver
        src/main.c
        src/parser.c
        src/include/parser.h
        src/mime.c
        src/include/mime.h
        src/static.c
        src/include/static.h
        src/listener.c
        src/include/listener.h
        src/util.c
        src/include/util.h
        src/server.c
        src/include/server.h
        src/response.c
        src/include/response.h
        src/connection.c
        src/include/connection.h
        src/include/request.h
        src/threads.c
        src/include/threads.h
        )

target_link_libraries(webserver pthread)