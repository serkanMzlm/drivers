add_library(drivers_comms
    i2c_transport.cpp
    spi_transport.cpp
    uart_transport.cpp
)

add_library(drivers::comms ALIAS drivers_comms)

target_include_directories(drivers_comms
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(drivers_comms
    PRIVATE i2c
)

target_compile_features(drivers_comms PUBLIC cxx_std_17)

target_compile_options(drivers_comms PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

install(TARGETS drivers_comms
    EXPORT driversTargets
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)