set(LIBRARY_NAME "libkissat")
set(EXEC_NAME "kissat")

set(Header_Files
        "allocate.h"
        "analyze.h"
        "ands.h"
        "arena.h"
        "array.h"
        "assign.h"
        "attribute.h"
        "autarky.h"
        "averages.h"
        "backbone.h"
        "backtrack.h"
        "backward.h"
        "bits.h"
        "bump.h"
        "cache.h"
        "check.h"
        "clause.h"
        "clueue.h"
        "collect.h"
        "colors.h"
        "compact.h"
        "config.h"
        "cover.h"
        "decide.h"
        "deduce.h"
        "definition.h"
        "dense.h"
        "dominate.h"
        "eliminate.h"
        "endianness.h"
        "equivalences.h"
        "error.h"
        "extend.h"
        "failed.h"
        "fastassign.h"
        "file.h"
        "flags.h"
        "format.h"
        "forward.h"
        "frames.h"
        "gates.h"
        "handle.h"
        "heap.h"
        "ifthenelse.h"
        "import.h"
        "inline.h"
        "inlineassign.h"
        "inlineframes.h"
        "inlineheap.h"
        "inlinequeue.h"
        "inlinereap.h"
        "inlinescore.h"
        "inlinevector.h"
        "internal.h"
        "kissat.h"
        "learn.h"
        "limits.h"
        "literal.h"
        "logging.h"
        "minimize.h"
        "mode.h"
        "nonces.h"
        "options.h"
        "parse.h"
        "phases.h"
        "print.h"
        "probe.h"
        "profile.h"
        "promote.h"
        "proof.h"
        "propdense.h"
        "prophyper.h"
        "proplit.h"
        "proprobe.h"
        "propsearch.h"
        "queue.h"
        "random.h"
        "rank.h"
        "reap.h"
        "reduce.h"
        "reference.h"
        "reluctant.h"
        "rephase.h"
        "report.h"
        "require.h"
        "resize.h"
        "resolve.h"
        "resources.h"
        "restart.h"
        "search.h"
        "shrink.h"
        "smooth.h"
        "sort.h"
        "stack.h"
        "statistics.h"
        "strengthen.h"
        "substitute.h"
        "sweep.h"
        "terminate.h"
        "ternary.h"
        "trail.h"
        "transitive.h"
        "utilities.h"
        "value.h"
        "vector.h"
        "vivify.h"
        "walk.h"
        "watch.h"
        "weaken.h"
        "witness.h"
        "xors.h"
)
source_group("Header Files" FILES "${Header_Files}")

set(Source_Files
        "allocate.c"
        "analyze.c"
        "ands.c"
        "arena.c"
        "assign.c"
        "autarky.c"
        "averages.c"
        "backbone.c"
        "backtrack.c"
        "backward.c"
        "bits.c"
        "build.c"
        "bump.c"
        "cache.c"
        "check.c"
        "clause.c"
        "clueue.c"
        "collect.c"
        "colors.c"
        "compact.c"
        "config.c"
        "decide.c"
        "deduce.c"
        "definition.c"
        "dense.c"
        "dominate.c"
        "dump.c"
        "eliminate.c"
        "equivalences.c"
        "error.c"
        "extend.c"
        "failed.c"
        "file.c"
        "flags.c"
        "format.c"
        "forward.c"
        "gates.c"
        "handle.c"
        "heap.c"
        "ifthenelse.c"
        "import.c"
        "internal.c"
        "learn.c"
        "limits.c"
        "logging.c"
        "minimize.c"
        "mode.c"
        "nonces.c"
        "options.c"
        "parse.c"
        "phases.c"
        "print.c"
        "probe.c"
        "profile.c"
        "promote.c"
        "proof.c"
        "propdense.c"
        "prophyper.c"
        "proprobe.c"
        "propsearch.c"
        "queue.c"
        "reap.c"
        "reduce.c"
        "reluctant.c"
        "rephase.c"
        "report.c"
        "resize.c"
        "resolve.c"
        "resources.c"
        "restart.c"
        "search.c"
        "shrink.c"
        "smooth.c"
        "sort.c"
        "stack.c"
        "statistics.c"
        "strengthen.c"
        "substitute.c"
        "sweep.c"
        "terminate.c"
        "ternary.c"
        "trail.c"
        "transitive.c"
        "utilities.c"
        "vector.c"
        "vivify.c"
        "walk.c"
        "watch.c"
        "weaken.c"
        "witness.c"
        "xors.c"
)
source_group("Source Files" FILES "${Source_Files}")

add_library("${LIBRARY_NAME}" "${Header_Files}" "${Source_Files}")
target_include_directories(
        "${LIBRARY_NAME}"
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:include>"
)
set_target_properties(
        "${LIBRARY_NAME}"
        PROPERTIES
        LINKER_LANGUAGE
        C
)

# install rules
set(installable_libs "${LIBRARY_NAME}" "${PROJECT_NAME}_compiler_flags")
install(FILES "${Header_Files}" DESTINATION "include")

set(exec_targets "application" "kitten" "main")
foreach (target ${exec_targets})
    set(EXEC_NAME "${PROJECT_NAME}_${target}")

    set(Source_Files "${target}.c")
    source_group("${target} Source Files" FILES "${Source_Files}")

    if (target STREQUAL "main")
        add_executable("${EXEC_NAME}" "${Source_Files}")
    else ()
        set(Header_Files "${target}.h")
        source_group("${target} Header Files" FILES "${Header_Files}")
        add_executable("${EXEC_NAME}" "${Header_Files}" "${Source_Files}")
        install(FILES "${Header_Files}" DESTINATION "include")
    endif ()

    target_include_directories(
            "${EXEC_NAME}"
            PRIVATE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
            "$<INSTALL_INTERFACE:include>"
    )
    set_target_properties(
            "${EXEC_NAME}"
            PROPERTIES
            LINKER_LANGUAGE
            C
    )
    list(APPEND installable_libs "${EXEC_NAME}")
endforeach ()

if (TARGET "${DEPENDANT_LIBRARY}")
    list(APPEND installable_libs "${DEPENDANT_LIBRARY}")
endif ()
install(TARGETS ${installable_libs}
        DESTINATION "bin"
        EXPORT "${EXEC_NAME}Targets")
