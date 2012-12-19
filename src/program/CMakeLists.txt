
include_directories(${CMPI_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)

add_executable(konkret main.cpp)
target_link_libraries(konkret konkretmof)

install(TARGETS konkret DESTINATION bin)
