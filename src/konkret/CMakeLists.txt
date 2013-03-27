
set(konkret_SRCS
    defaultassoc.c
    defaultei.c
    defaultein.c
    defaultgi.c
    general.c
    kstr.c
    print.c
)

include_directories(${CMPI_INCLUDE_DIR})

add_library(libkonkret SHARED ${konkret_SRCS})

set_target_properties(libkonkret PROPERTIES VERSION 0.0.1 OUTPUT_NAME konkret)
set_target_properties(libkonkret PROPERTIES SOVERSION 0 OUTPUT_NAME konkret)

install(TARGETS libkonkret DESTINATION lib${LIB_SUFFIX})
install(FILES konkret.h DESTINATION include/konkret)
