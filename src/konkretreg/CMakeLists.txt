
include_directories(${CMPI_INCLUDE_DIR})

add_executable(konkretreg
    main.cpp
)

install(TARGETS konkretreg DESTINATION bin)
