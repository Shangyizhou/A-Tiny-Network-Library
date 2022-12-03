add_executable(MemoryPoolTest MemoryPoolTest.cc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/memory/test)

target_link_libraries(MemoryPoolTest tiny_network)
