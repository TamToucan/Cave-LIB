# Target Name
set(CAVE_LIB_NAME cave)

# Source Files
file(GLOB_RECURSE CAVE_SOURCES CONFIGURE_DEPENDS
    "*.h"
    "*.hpp"
    "*.cpp"
)

# Define Shared Library
add_library(${CAVE_LIB_NAME} SHARED ${CAVE_SOURCES})

# Include Directories
# Note: adjusted paths relative to 'cave/src/core/'
target_include_directories(${CAVE_LIB_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"       # . (core)
    "${CMAKE_CURRENT_SOURCE_DIR}/.."    # .. (src)
    # These hardcoded dependency paths should match where FetchContent puts 'Libs'
    "${CMAKE_BINARY_DIR}/_deps/libs-src/lib/MathStuff"
    "${CMAKE_BINARY_DIR}/_deps/libs-src/lib/Algo"
    "${CMAKE_BINARY_DIR}/_deps/libs-src/lib/Util"
)

# Properties
set_target_properties(${CAVE_LIB_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(NOT MSVC)
    set_target_properties(${CAVE_LIB_NAME} PROPERTIES PREFIX "")
endif()

# Dependencies
# Assumes 'Libs' has been fetched by the Root CMake
target_link_libraries(${CAVE_LIB_NAME}
    PRIVATE Algo
    PRIVATE PCG
    PRIVATE Random
    PRIVATE MathStuff
)
