cmake_minimum_required(VERSION 3.14)

project(drivers
    VERSION 0.1.0
    DESCRIPTION "A reusable C++ utility library: math, logging, filters"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DRIVERS_BUILD_TESTS "Build the unit tests" ON)

add_subdirectory(src/comms)

if(DRIVERS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(DIRECTORY include/drivers
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT driversTargets
    FILE driversTargets.cmake
    NAMESPACE drivers::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/drivers
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/driversConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/driversConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/drivers
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/driversConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/driversConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/driversConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/drivers
)

if(NOT TARGET uninstall)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/driversUninstall.cmake
        COMMENT "Uninstalling drivers (removing installed files)..."
    )
endif()