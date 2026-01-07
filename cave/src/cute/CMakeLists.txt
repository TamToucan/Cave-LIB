
add_library(CuteCave SHARED)

target_sources(CuteCave PRIVATE
	CuteCave.cpp
)

target_include_directories(CuteCave PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(CuteCave PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../core")

target_link_libraries(CuteCave
    PRIVATE Cave
    PRIVATE cute_lib
)
# Explicitly add cute include directory as a fallback
target_include_directories(CuteCave PRIVATE "${CMAKE_SOURCE_DIR}/deps/cute/include")
target_include_directories(CuteCave PRIVATE "${CMAKE_SOURCE_DIR}/deps/cute")

set_target_properties(CuteCave PROPERTIES OUTPUT_NAME CuteCave)
set_target_properties(CuteCave PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(NOT MSVC)
	set_target_properties(CuteCave PROPERTIES PREFIX "")
endif()

# Windows: Copy DLL next to CuteCave so tests can run
add_custom_command(TARGET CuteCave POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:Cave>
    $<TARGET_FILE_DIR:CuteCave>
)
