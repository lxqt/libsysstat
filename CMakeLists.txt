cmake_minimum_required(VERSION 2.8.11)

project(libsysstat)

# Set default installation paths
set(LIB_INSTALL_DIR "lib${LIB_SUFFIX}" CACHE PATH "Installation path for libraries")
set(INCLUDE_INSTALL_DIR include CACHE PATH "Installation path for includes")

set(MAJOR_VERSION 0)
set(MINOR_VERSION 2)
set(PATCH_VERSION 0)
set(SYSSTAT_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")

add_definitions(-DMAJOR_VERSION=${MAJOR_VERSION})
add_definitions(-DMINOR_VERSION=${MINOR_VERSION})
add_definitions(-DPATCH_VERSION=${PATCH_VERSION})
add_definitions(-DSYSSTAT_LIBRARY)

# use gcc visibility feature to decrease unnecessary exported symbols
if(CMAKE_COMPILER_IS_GNUCXX)
    # set visibility to hidden to hide symbols, unlesss they're exporeted manually in the code
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fno-exceptions -Wall")
endif()

find_package(Qt5Core REQUIRED QUIET)
find_package(Qt5LinguistTools REQUIRED QUIET)

set(SYSSTAT_LIBRARY_NAME "sysstat-qt5")
set(HEADER_NAMESPACE "SysStat")

set(SYSSTAT_PUBLIC_HDRS
    version.h
    sysstat_global.h
    basestat.h
    cpustat.h
    memstat.h
    netstat.h
)

set(SYSSTAT_PUBLIC_CLASSES
    Version
    Global
    BaseStat
    CpuStat
    MemStat
    NetStat
)

set(SYSSTAT_PRIVATE_HDRS
    version_p.h
    basestat_p.h
    cpustat_p.h
    memstat_p.h
    netstat_p.h
)

set(SYSSTAT_SRCS
    version.cpp
    basestat.cpp
    cpustat.cpp
    memstat.cpp
    netstat.cpp
)

set(SYSSTAT_MOCS
    basestat.h
    cpustat.h
    memstat.h
    netstat.h
    basestat_p.h
    cpustat_p.h
    memstat_p.h
    netstat_p.h
)


# Translations
file(GLOB SYSSTAT_TS_FILES
    translations/*.ts
)

qt5_wrap_cpp(SYSSTAT_CXX ${SYSSTAT_MOCS})
qt5_add_translation(SYSSTAT_QM_FILES ${SYSSTAT_TS_FILES})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()


set(APP_SHARE_DIR "${CMAKE_INSTALL_PREFIX}/share/${SYSSTAT_LIBRARY_NAME}")

include_directories(..)
add_definitions(-DTRANSLATIONS_DIR=\"${APP_SHARE_DIR}\")

add_library(${SYSSTAT_LIBRARY_NAME} SHARED ${SYSSTAT_PUBLIC_HDRS} ${SYSSTAT_PRIVATE_HDRS} ${SYSSTAT_SRCS} ${SYSSTAT_CXX} ${SYSSTAT_QM_FILES})

target_link_libraries(${SYSSTAT_LIBRARY_NAME}  Qt5::Core)

set_target_properties(${SYSSTAT_LIBRARY_NAME} PROPERTIES
  VERSION ${SYSSTAT_VERSION}
  SOVERSION ${MAJOR_VERSION}
)

include(cmake/create_portable_headers.cmake)

create_portable_headers(SYSSTAT_NAMESPACE_HEADERS
    NAMESPACE "${HEADER_NAMESPACE}/"
    FILENAMES ${SYSSTAT_PUBLIC_CLASSES}
)
install(TARGETS ${SYSSTAT_LIBRARY_NAME} DESTINATION "${LIB_INSTALL_DIR}")
install(FILES ${SYSSTAT_PUBLIC_HDRS} DESTINATION "${INCLUDE_INSTALL_DIR}/${SYSSTAT_LIBRARY_NAME}")
install(FILES ${SYSSTAT_NAMESPACE_HEADERS} DESTINATION "${INCLUDE_INSTALL_DIR}/${SYSSTAT_LIBRARY_NAME}/${HEADER_NAMESPACE}")
install(FILES ${SYSSTAT_QM_FILES} DESTINATION "${APP_SHARE_DIR}")

include(cmake/create_pkgconfig_file.cmake)
create_pkgconfig_file(${SYSSTAT_LIBRARY_NAME} "SysStat, a Qt-based interface to system statistics")

# cmake config file
set(SYSSTAT_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include/${SYSSTAT_LIBRARY_NAME}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sysstat-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${SYSSTAT_LIBRARY_NAME}-config.cmake"
    @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${SYSSTAT_LIBRARY_NAME}-config.cmake" DESTINATION "share/cmake/${SYSSTAT_LIBRARY_NAME}")
install(FILES cmake/sysstat_use.cmake DESTINATION "share/cmake/${SYSSTAT_LIBRARY_NAME}")

# building tarball with CPack -------------------------------------------------
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${PATCH_VERSION})
set(CPACK_GENERATOR TBZ2)
set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include(CPack)
