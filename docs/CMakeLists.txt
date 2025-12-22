find_package(Doxygen REQUIRED)

set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
set(DOXYGEN_EXCLUDE_SYMBOLS "FLEXI_IMPL_*")

doxygen_add_docs(flexic_docs
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/flexic.h"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../include")
