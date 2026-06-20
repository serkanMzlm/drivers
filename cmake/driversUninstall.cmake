# ═══════════════════════════════════════════════════════════════════
#  Safely removes every file that was recorded during installation.
#  It only touches paths listed in install_manifest.txt nothing is
#  guessed, so no unrelated file is ever deleted.
# ═══════════════════════════════════════════════════════════════════

set(MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")

if(NOT EXISTS "${MANIFEST}")
    message(FATAL_ERROR
        "Cannot uninstall: install_manifest.txt not found. "
        "Was the project actually installed from this build directory?")
endif()

file(STRINGS "${MANIFEST}" INSTALLED_FILES)

foreach(FILE ${INSTALLED_FILES})
    if(EXISTS "${FILE}" OR IS_SYMLINK "${FILE}")
        message(STATUS "Removing: ${FILE}")
        file(REMOVE "${FILE}")
        if(EXISTS "${FILE}")
            message(WARNING "Failed to remove: ${FILE}")
        endif()
    else()
        message(STATUS "Already gone: ${FILE}")
    endif()
endforeach()

message(STATUS "drivers uninstall complete.")