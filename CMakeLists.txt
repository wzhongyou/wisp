cmake_minimum_required(VERSION 3.16)

project(wisp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(FetchContent)
find_package(Threads REQUIRED)

add_library(wisp_core
    src/connection.cpp
    src/parser.cpp
    src/server.cpp
    src/store.cpp
)

target_include_directories(wisp_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(wisp_core PUBLIC cxx_std_17)
target_link_libraries(wisp_core PUBLIC Threads::Threads)

add_executable(wisp_server src/main.cpp)
target_link_libraries(wisp_server PRIVATE wisp_core)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
