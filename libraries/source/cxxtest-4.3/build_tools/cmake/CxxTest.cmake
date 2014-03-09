
include("${CMAKE_CURRENT_LIST_DIR}/FindCxxTest.cmake")

function(cxx_test target source)
    string(REGEX REPLACE "hpp$" "cpp" CPP_FILE_NAME ${source})
    message(${CPP_FILE_NAME})
    set(CPP_FULL_NAME "${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE_NAME}")
    add_custom_command(
        OUTPUT "${CPP_FULL_NAME}"
        COMMAND ${CXXTESTGEN} --runner=ErrorPrinter --output "${CPP_FULL_NAME}" "${source}"
        DEPENDS "${source}"
    )
    add_executable(${target} ${CPP_FULL_NAME})
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS "-Wno-effc++")
    add_test(${target} ${RUNTIME_OUTPUT_DIRECTORY}/${target})
endfunction(cxx_test)
