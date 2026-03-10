
add_library(CuteCave STATIC)

target_sources(CuteCave PRIVATE
	CuteCave.cpp
)

target_include_directories(CuteCave PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(CuteCave PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../core")

target_link_libraries(CuteCave
    PRIVATE Cave
    PRIVATE cute_lib
)

# Windows: Copy DLL next to CuteCave so tests can run
add_custom_command(TARGET CuteCave POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:Cave>
    $<TARGET_FILE_DIR:CuteCave>
)
