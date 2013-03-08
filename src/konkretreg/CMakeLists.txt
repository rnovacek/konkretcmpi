
include_directories(${CMPI_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/)

add_executable(konkretreg
    main.cpp
)

install(TARGETS konkretreg DESTINATION bin)
