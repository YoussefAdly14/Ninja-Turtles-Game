cmake_minimum_required(VERSION 3.18)
project(NinjaTurtlesGame)

# Based on the course VS Code starter (context/reference/starter/CMakeLists.txt).

# skip the parts of the libraries we don't use (faster build, no extra tools needed)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB     ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON  CACHE BOOL "" FORCE)

# third-party libraries (GLAD, GLFW, GLM, FreeType) -- change this one line if the folder moves
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/context/external)

add_subdirectory(${EXTERNAL_DIR}/glfw-3.5.1 glfw)
add_subdirectory(${EXTERNAL_DIR}/glm glm)
add_subdirectory(${EXTERNAL_DIR}/freetype freetype)   # text rendering (learnopengl In-Practice/Text-Rendering)

# FreeType's Windows version-info resource breaks MinGW's windres when the path has a space
# (e.g. C:/Users/Some Name/...). It only matters for a FreeType DLL; we link it statically.
set_source_files_properties(${EXTERNAL_DIR}/freetype/src/base/ftver.rc
    DIRECTORY ${EXTERNAL_DIR}/freetype PROPERTIES HEADER_FILE_ONLY TRUE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# Every .cpp under src/ is picked up automatically: add new files without editing this list
# (re-run CMake configure after adding a file).
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(NinjaTurtlesGame ${GAME_SOURCES})

target_include_directories(NinjaTurtlesGame PRIVATE src ${EXTERNAL_DIR}/glad/include)
target_link_libraries(NinjaTurtlesGame PRIVATE glfw OpenGL::GL glm::glm freetype)

# MinGW: bake the C++ runtime into the .exe so it also runs outside VS Code
if(MINGW)
    target_link_options(NinjaTurtlesGame PRIVATE -static)
endif()

# Copy assets/ (shaders, fonts, sounds) next to the .exe so relative paths like
# "assets/shaders/basic.vert" work whether the game starts from the build folder or the repo root.
add_custom_command(TARGET NinjaTurtlesGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:NinjaTurtlesGame>/assets)
