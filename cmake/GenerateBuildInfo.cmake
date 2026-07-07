# Writes build_info.h with the current git branch/commit. Runs on every build,
# but only touches the file when the content changes so incremental builds
# stay incremental. Falls back to "unknown" outside a git checkout.
find_package(Git QUIET)
set(info "unknown")
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${SRC_DIR} OUTPUT_VARIABLE branch
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE r1)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${SRC_DIR} OUTPUT_VARIABLE hash
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE r2)
    execute_process(COMMAND ${GIT_EXECUTABLE} status --porcelain --untracked-files=no
        WORKING_DIRECTORY ${SRC_DIR} OUTPUT_VARIABLE dirty ERROR_QUIET RESULT_VARIABLE r3)
    if(r1 EQUAL 0 AND r2 EQUAL 0)
        set(info "${branch} @ ${hash}")
        if(r3 EQUAL 0 AND NOT dirty STREQUAL "")
            string(APPEND info " (modified)")
        endif()
    endif()
endif()
set(content "#pragma once\n#define SMX_GIF_MAKER_BUILD \"${info}\"\n")
set(old "")
if(EXISTS ${OUT_FILE})
    file(READ ${OUT_FILE} old)
endif()
if(NOT content STREQUAL old)
    file(WRITE ${OUT_FILE} ${content})
endif()
