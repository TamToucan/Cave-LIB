# Target Name
set(GDCAVE_LIB_NAME GDCave)

# Source Files
file(GLOB_RECURSE GDCAVE_SOURCES CONFIGURE_DEPENDS
    "*.h"
    "*.hpp"
    "*.cpp"
)

# Define Shared Library
add_library(${GDCAVE_LIB_NAME} SHARED ${GDCAVE_SOURCES})
target_compile_definitions(${GDCAVE_LIB_NAME} PRIVATE GDCAVE_EXPORTS)

# Include Directories
target_include_directories(${GDCAVE_LIB_NAME}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../core" # Access to sibling 'core'
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"         # . (godot)
    PRIVATE "${CMAKE_BINARY_DIR}/_deps/libs-src/lib/Util"
)

# Properties
set_target_properties(${GDCAVE_LIB_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(NOT MSVC)
    set_target_properties(${GDCAVE_LIB_NAME} PROPERTIES PREFIX "")
endif()

# Linking
# Links against the sibling 'cave' target defined in src/core
target_link_libraries(${GDCAVE_LIB_NAME}
    PRIVATE Cave
    PUBLIC godot::cpp
)

# Post-Build Copy (Kept from your original)
# Note: You might want to make the destination path a variable in the Root CMake 
# so it isn't hardcoded to "D:/git/the-lott..." for other users.
add_custom_command(
    TARGET ${GDCAVE_LIB_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:Cave>
        "D:/git/the-lott/addons/gdcave/bin"
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${GDCAVE_LIB_NAME}>
        "D:/git/the-lott/addons/gdcave/bin"
)
