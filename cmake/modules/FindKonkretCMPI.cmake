
find_path(KONKRETCMPI_INCLUDE_DIR
    NAMES konkret.h
    HINTS $ENV{KONKTRETCMPI_INCLUDE_DIR}
    PATH_SUFFIXES include/konkret include
    PATHS /usr /usr/local
)

find_library(KONKRETCMPI_LIBRARY
    NAMES konkret
    HINTS $ENV{KONKRETCMPI_LIB_DIR}
    PATH_SUFFIXES lib64 lib
    PATHS /usr /usr/local
)

find_program(KONKRETCMPI_KONKRET
    NAMES konkret
    HINTS $ENV{KONKRETCMPI_BIN_DIR}
    PATH_SUFFIXES bin
    PATHS /usr/ /usr/local
)

find_program(KONKRETCMPI_KONKRETREG
    NAMES konkretreg
    HINTS $ENV{KONKRETCMPI_BIN_DIR}
    PATH_SUFFIXES bin
    PATHS /usr/ /usr/local
)

set(KONKRETCMPI_LIBRARIES ${KONKRETCMPI_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KonkretCMPI DEFAULT_MSG KONKRETCMPI_LIBRARIES KONKRETCMPI_INCLUDE_DIR)

mark_as_advanced(KONKRETCMPI_INCLUDE_DIR KONKRETCMPI_LIBRARIES)
