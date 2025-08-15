cmake_minimum_required(VERSION 3.10)
project(HapticLib)

# Find pybind11
find_package(pybind11 REQUIRED)

# Find HDAPI (adjust paths as needed)
find_library(HD_LIBRARY NAMES HD PATHS "/path/to/hdapi/lib")
include_directories("${CMAKE_SOURCE_DIR}/include" "/path/to/hdapi/include")

# Ensure C++11 or later for std::array
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build the haptic library with -fPIC
add_library(haptic_lib STATIC src/haptic_lib.cpp)
set_target_properties(haptic_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(haptic_lib ${HD_LIBRARY})

# Build the Python module
pybind11_add_module(haptic src/haptic_binding.cpp)
target_link_libraries(haptic PRIVATE haptic_lib ${HD_LIBRARY})

# Ensure the output directory is accessible to Python
set_target_properties(haptic PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples")