cmake_minimum_required(VERSION 3.9)
project(LuoguPlaybackDownloader)

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14")

add_executable(main
    OJTool/Lib/Exception.cpp
    OJTool/Lib/Curl.cpp
    OJTool/Lib/StringOperation.cpp
    main.cpp)
target_include_directories(main PUBLIC OJTool/Lib)

find_library(LIBCURL curl REQUIRED)
set(LIBS
    ${LIBCURL}
)
target_link_libraries(main PRIVATE ${LIBS})
