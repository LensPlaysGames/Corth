cmake_minimum_required (VERSION 3.8)

project (Corth VERSION 0.0.1)

set(SOURCE_FILES 
	"src/Corth.cpp"
)

set(HEADER_FILES )

# Add source files project.
add_executable (Corth ${HEADER_FILES} ${SOURCE_FILES})

# Include necessary headers
# target_include_directories(Corth PUBLIC ${CMAKE_SOURCE_DIR}/path/to/headers)

# Link necessary libraries
# target_link_libraries(Corth name_of_cmake_library)

# Copy resources in directory
#add_custom_command(TARGET Corth POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#        "${PROJECT_SOURCE_DIR}/path/to/resources"
#        $<TARGET_FILE_DIR:Corth>)