set(SRC_LIST ./muduo_server.cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(server ${SRC_LIST})

target_link_libraries(server muduo_net muduo_base pthread)