# Copyright (c) 2024, Lexi Mayfield
# All rights reserved.
#
# Based on CodeCoverage.cmake
# Copyright (c) 2012 - 2017, Lars Bilke
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

include(CMakeParseArguments)

find_program( LLVMCOV_PATH llvm-cov )
find_program( LLVMPROFDATA_PATH llvm-profdata )
find_program( LLVMCXXFILT_PATH llvm-cxxfilt )

set(LLVMCOV_COMPILER_FLAGS "-fprofile-instr-generate -fcoverage-mapping"
    CACHE INTERNAL "")

function(setup_target_for_coverage_llvmcov)
    set(options "")
    set(oneValueArgs BASE_DIRECTORY BUILD_DIRECTORY NAME OBJECT)
    set(multiValueArgs SOURCES EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
    cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT LLVMCOV_PATH)
        message(FATAL_ERROR "llvm-cov not found! Aborting...")
    endif() # NOT LLVMCOV_PATH

    if(NOT LLVMPROFDATA_PATH)
        message(FATAL_ERROR "llvm-profdata not found! Aborting...")
    endif() # NOT LLVMPROFDATA_PATH

    if(NOT LLVMCXXFILT_PATH)
        message(FATAL_ERROR "llvm-cxxfilt not found! Aborting...")
    endif() # NOT LLVMCXXFILT_PATH

    # Set base directory (as absolute path), or default to PROJECT_SOURCE_DIR
    if(DEFINED Coverage_BASE_DIRECTORY)
        get_filename_component(BASEDIR ${Coverage_BASE_DIRECTORY} ABSOLUTE)
    else()
        set(BASEDIR ${PROJECT_SOURCE_DIR})
    endif()

    set(LLVMCOV_EXEC_TESTS_CMD
        "$<TARGET_FILE:${Coverage_EXECUTABLE}>" ${Coverage_EXECUTABLE_ARGS})

    set(LLVMCOV_MERGE_CMD
        "${LLVMPROFDATA_PATH}" merge -sparse default.profraw
            -o default.profdata)

    set(LLVMCOV_SHOW_CMD
        "${LLVMCOV_PATH}" show
            "-Xdemangler=${LLVMCXXFILT_PATH}"
            -format=html
            "-o=${Coverage_NAME}"
            -instr-profile=default.profdata 
            -object "$<TARGET_FILE:${Coverage_OBJECT}>"
            -sources ${Coverage_SOURCES})

    if(TRUE)
        message(STATUS "Executed command report")
        message(STATUS "Command to execute tests: ")
        string(REPLACE ";" " " LLVMCOV_EXEC_TESTS_CMD_SPACED "${LLVMCOV_EXEC_TESTS_CMD}")
        message(STATUS "${LLVMCOV_EXEC_TESTS_CMD_SPACED}")

        message(STATUS "Command to merge profile data: ")
        string(REPLACE ";" " " LLVMCOV_MERGE_CMD_SPACED "${LLVMCOV_MERGE_CMD}")
        message(STATUS "${LLVMCOV_MERGE_CMD_SPACED}")

        message(STATUS "Command to generate report: ")
        string(REPLACE ";" " " LLVMCOV_SHOW_CMD_SPACED "${LLVMCOV_SHOW_CMD}")
        message(STATUS "${LLVMCOV_SHOW_CMD_SPACED}")
    endif()

    # Setup target
    add_custom_target(${Coverage_NAME}
        COMMAND ${LLVMCOV_EXEC_TESTS_CMD}
        COMMAND ${LLVMCOV_MERGE_CMD}
        COMMAND ${LLVMCOV_SHOW_CMD}

        BYPRODUCTS
            default.profdata
            default.profraw
            "${Coverage_NAME}/index.html"

        WORKING_DIRECTORY "${BUILDDIR}"
        DEPENDS ${Coverage_DEPENDENCIES}
        VERBATIM # Protect arguments to commands
        COMMENT "Processing code coverage counters and generating report.")
endfunction()

function(append_llvmcov_compiler_flags_to_target name)
    separate_arguments(_flag_list NATIVE_COMMAND "${LLVMCOV_COMPILER_FLAGS}")
    target_compile_options(${name} PRIVATE ${_flag_list})
    target_link_options(${name} PRIVATE ${_flag_list})
endfunction()
