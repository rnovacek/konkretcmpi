
find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

set(Python_ADDITIONAL_VERSIONS 2.7 2.6 2.5 2.4)
find_package(PythonLibs REQUIRED)
string(REGEX REPLACE "^([0-9]+\\.[0-9]+).*" "\\1" PYTHON_VER "${PYTHONLIBS_VERSION_STRING}")

include_directories(${PYTHON_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(CMAKE_SWIG_FLAGS "-c++")

set_source_files_properties(konkretmof.i PROPERTIES CPLUSPLUS ON)
set_source_files_properties(konkretmof.i PROPERTIES SWIG_FLAGS "-includeall")
swig_add_module(konkretmof python
  konkretmof.i)
swig_link_libraries(konkretmof ${PYTHON_LIBRARIES} konkretmof)

install(TARGETS ${SWIG_MODULE_konkretmof_REAL_NAME} DESTINATION lib${LIB_SUFFIX}/python${PYTHON_VER}/site-packages)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/konkretmof.py DESTINATION lib${LIB_SUFFIX}/python${PYTHON_VER}/site-packages)
