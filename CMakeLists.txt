cmake_minimum_required(VERSION 3.16)
project(DawProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Fetch SFML
message(STATUS "Fetching SFML...")
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
)
FetchContent_MakeAvailable(sfml)

# Create ALIAS targets so ImGui-SFML can find them
if(TARGET sfml-graphics AND NOT TARGET SFML::Graphics)
    add_library(SFML::Graphics ALIAS sfml-graphics)
    add_library(SFML::Window ALIAS sfml-window)
    add_library(SFML::System ALIAS sfml-system)
    add_library(SFML::Audio ALIAS sfml-audio)
endif()

# 2. Fetch ImGui
message(STATUS "Fetching ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

# 3. Fetch ImGui-SFML
message(STATUS "Fetching ImGui-SFML...")
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    imgui_sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v3.0
)
FetchContent_MakeAvailable(imgui_sfml)

# 4. Our Executable
add_executable(DawProject src/main.cpp)

# Include vendor directory for miniaudio.h
target_include_directories(DawProject PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

# Link everything together
target_link_libraries(DawProject PRIVATE 
    sfml-graphics sfml-window sfml-system sfml-audio
    ImGui-SFML::ImGui-SFML
)
