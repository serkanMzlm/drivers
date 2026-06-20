function(drivers_add_test name source)
    cmake_parse_arguments(ARG "MANUAL" "" "" ${ARGN})

    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE Threads::Threads ${ARG_UNPARSED_ARGUMENTS})

    if(NOT ARG_MANUAL)
        add_test(NAME ${name} COMMAND ${name})
        set_tests_properties(${name} PROPERTIES TIMEOUT 30)
    endif()
endfunction()

find_package(Threads REQUIRED)

# omnikit_add_test(test_log test_log.cpp omnikit::log)
