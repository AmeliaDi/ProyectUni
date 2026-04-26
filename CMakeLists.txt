cmake_minimum_required(VERSION 3.14)
project(app_drogon CXX)

# Establecer el estándar mínimo de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)