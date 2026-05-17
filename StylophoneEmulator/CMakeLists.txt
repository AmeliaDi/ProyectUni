cmake_minimum_required(VERSION 3.24)
project(StylophoneEmulator VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find system dependencies
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# math library (for miniaudio/synthesis)
find_library(MATH_LIBRARY m)

# Fetch Dear ImGui
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.5
)
FetchContent_MakeAvailable(imgui)

# Create an interface library for imgui since it's just source files
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_lib PUBLIC 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC SDL2::SDL2 OpenGL::GL)

# Add our source files
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/StylophoneSynth.cpp
    src/Sequencer.cpp
)

# Includes
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Link
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui_lib
    SDL2::SDL2
    OpenGL::GL
    Threads::Threads
)

if(MATH_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MATH_LIBRARY})
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()
