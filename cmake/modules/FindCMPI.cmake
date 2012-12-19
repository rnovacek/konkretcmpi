
find_path(CMPI_INCLUDE_DIR
    NAMES cmpidt.h cmpift.h cmpimacs.h cmpios.h cmpipl.h
    HINTS $ENV{CMPI_INCLUDE_DIR}
    PATH_SUFFIXES include/cmpi include
    PATHS /usr /usr/local
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CMPI DEFAULT_MSG CMPI_INCLUDE_DIR)

mark_as_advanced(CMPI_INCLUDE_DIR)
