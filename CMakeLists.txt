cmake_minimum_required(VERSION 3.19)
project(LasCorto)

find_package(laszip REQUIRED)

add_subdirectory("corto")

add_executable(main src/bin/main.cpp)
add_executable(decode src/bin/decode.cpp)
target_link_libraries(main corto laszip::laszip)
target_compile_features(main PUBLIC cxx_std_20)
target_link_libraries(decode corto laszip::laszip)
target_compile_features(decode PUBLIC cxx_std_20)