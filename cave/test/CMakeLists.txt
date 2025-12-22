# Define the test executable
add_executable(cave_test main.cpp)

# Include Directories
# Needs access to 'src' to include headers like "core/Cave.h"
target_include_directories(cave_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)

# Link Dependencies
# Links against the 'cave' library (defined in src/core)
target_link_libraries(cave_test PRIVATE CaveLib::Cave)

# (Optional) Ensure the DLLs are copied next to the test executable for Windows
if(TARGET CaveLib::Cave)
    add_custom_command(TARGET cave_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:CaveLib::Cave>
        $<TARGET_FILE_DIR:cave_test>
    )
endif()
