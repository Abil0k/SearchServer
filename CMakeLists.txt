cmake_minimum_required(VERSION 3.0.0)
project(SearchServer VERSION 0.1.0)

add_executable(Main main.cpp document.cpp process_queries.cpp read_input_functions.cpp remove_duplicates.cpp request_queue.cpp
search_server.cpp string_processing.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -g -Wall -O3 -ltbb -lpthread")

