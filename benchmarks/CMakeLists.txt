set(TARGET_NAME "benchmark")
find_package(benchmark REQUIRED)
add_executable(${TARGET_NAME} ${TARGET_NAME}.cpp)
target_link_libraries(${TARGET_NAME} ITCH_lib benchmark::benchmark ZLIB::ZLIB)
