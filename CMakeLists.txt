# Set the minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(Challenge)

# Set the C++ standard to use
set(CMAKE_CXX_STANDARD 11)

# Add a source file to the project
add_executable(Challenge Challenge/Challenge.cpp)

set(SQLite3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set(SQLite3_LIBRARY Challenge/sqlite3.lib)

# Add SQLite library
find_package(SQLite3 REQUIRED)
target_link_libraries(Challenge SQLite::SQLite3)

# Add Winsock library
target_link_libraries(Challenge ws2_32)
