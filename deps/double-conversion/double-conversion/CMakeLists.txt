set(headers
  bignum.h
  cached-powers.h
  diy-fp.h
  double-conversion.h
  fast-dtoa.h
  fixed-dtoa.h
  ieee.h
  strtod.h
  utils.h
  )

add_library(double-conversion
bignum.cc
bignum-dtoa.cc
cached-powers.cc
diy-fp.cc
double-conversion.cc
fast-dtoa.cc
fixed-dtoa.cc
strtod.cc
${headers}
)

target_include_directories(double-conversion PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

# Add fPIC on x86_64 when supported.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-fPIC CXX_HAS_FPIC)

if(CXX_HAS_FPIC AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set_target_properties(double-conversion PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

#
# associates the list of headers with the library
# for the purposes of installation/import into other projects
set_target_properties(double-conversion
    PROPERTIES PUBLIC_HEADER "${headers}")

if (BUILD_SHARED_LIBS)
  set_target_properties(double-conversion
    PROPERTIES VERSION ${double-conversion_SOVERSION}
               SOVERSION ${double-conversion_SOVERSION_MAJOR})
endif()

#
# install command to set up library install
# given the above PUBLIC_HEADER property set, this
# pulls along all the header files with the library.
install(TARGETS double-conversion
  EXPORT double-conversionLibraryDepends
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/double-conversion"
  COMPONENT dev)
