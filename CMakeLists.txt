cmake_minimum_required(VERSION 3.16)

project(PixerShaper VERSION 1.0.0 LANGUAGES CXX)

# Include CPM (CMake Package Manager)
include(cmake/CPM.cmake)

# Add nativefiledialog-extended dependency
CPMAddPackage(
    NAME nativefiledialog_extended
    GITHUB_REPOSITORY btzy/nativefiledialog-extended
    GIT_TAG v1.2.1
)

if (nativefiledialog_extended_ADDED)
    add_library(nativefiledialog_extended INTERFACE IMPORTED)
    target_include_directories(nativefiledialog_extended INTERFACE ${nativefiledialog_extended_SOURCE_DIR}/src/include)
endif()

# Add nlohmann's JSON dependency
CPMAddPackage(
    NAME nlohmann_json
    VERSION 3.12.0
    # the git repo is incredibly large, so we download the archived include directory
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/include.zip
    URL_HASH SHA256=b8cb0ef2dd7f57f18933997c9934bb1fa962594f701cd5a8d3c2c80541559372
)

if (nlohmann_json_ADDED)
    add_library(nlohmann_json INTERFACE IMPORTED)
    target_include_directories(nlohmann_json INTERFACE ${nlohmann_json_SOURCE_DIR}/include)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Unicode support
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    add_compile_definitions(UNICODE _UNICODE)
endif()

# Platform detection and OpenGL setup
find_package(OpenGL REQUIRED)

# Platform-specific libraries
if(WIN32)
    # Windows-specific libraries
    set(PLATFORM_LIBS opengl32 winmm user32 gdi32 dwmapi)
    
elseif(UNIX AND NOT APPLE)
    # Linux-specific libraries
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    
    set(PLATFORM_LIBS ${OPENGL_LIBRARIES} ${X11_LIBRARIES} Threads::Threads pthread stdc++fs)
    
elseif(APPLE)
    # macOS-specific libraries
    find_library(OPENGL_FRAMEWORK OpenGL)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    
    set(PLATFORM_LIBS ${OPENGL_FRAMEWORK} ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK} ${COREVIDEO_FRAMEWORK})
endif()

# Include directories
include_directories(src)

# Add the main executable
add_executable(${PROJECT_NAME} 
    src/gui.cpp
    src/shaper.cpp
    src/main.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} ${PLATFORM_LIBS} nfd nlohmann_json)

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    # Enable Unicode support for MSVC
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    # Enable UTF-8 for GCC/Clang
    target_compile_options(${PROJECT_NAME} PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    
    # Additional flags for debug builds
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-g>)
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:-O3>)
endif()

# Platform-specific compile definitions
if(APPLE)
    # Find and link GLUT
    find_package(GLUT REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${GLUT_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${GLUT_INCLUDE_DIRS})
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
)

# Copy assets folder to output directory (post-build command)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets to output directory"
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION build
)

# Print configuration information
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Platform libs: ${PLATFORM_LIBS}")
