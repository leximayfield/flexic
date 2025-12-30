function(copy_to_target)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs GLOB)
    cmake_parse_arguments(PARSE_ARGV 0 "" "${options}" "${oneValueArgs}" "${multiValueArgs}")

    file(GLOB _files
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_GLOB}")

    foreach(_file ${_FILES})
        add_custom_command(
            TARGET ${_TARGET} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/${_file}"
                    "${CMAKE_CURRENT_BINARY_DIR}/${_file}"
            VERBATIM)
    endforeach()
endfunction()
