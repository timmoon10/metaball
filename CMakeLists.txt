cmake_minimum_required(VERSION 3.12)
project(metaball LANGUAGES CXX)

# Configure CMake
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Configure C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Dependencies
find_package(OpenMP REQUIRED CXX)
find_package(Qt6 REQUIRED Core Widgets)

# Configure executable
set(SOURCE_FILES
    src/metaball/main.cpp
    )
add_executable(metaball ${SOURCE_FILES})
include_directories(include)
target_link_libraries(metaball
                      PRIVATE
                      OpenMP::OpenMP_CXX
                      Qt6::Core
                      Qt6::Widgets
                      )

# Install library
install(TARGETS metaball DESTINATION .)
