cmake_minimum_required(VERSION 3.14)
project(qpdf_poc)

set(CMAKE_CXX_STANDARD 17)

add_executable(qpdf_poc main.cpp)
target_link_libraries(qpdf_poc qpdf)
