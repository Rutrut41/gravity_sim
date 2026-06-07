cmake_minimum_required(VERSION 3.10)
project(gravity_sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Include directories
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_DIRS})
include_directories(${glm_INCLUDE_DIRS})

# Main gravity simulation executable
add_executable(gravity_sim gravity_sim.cpp)
target_link_libraries(gravity_sim
    ${OPENGL_LIBRARIES}
    GLEW::GLEW
    glfw
)

# 3D Grid simulation executable
add_executable(gravity_sim_3d gravity_sim_3Dgrid.cpp)
target_link_libraries(gravity_sim_3d
    ${OPENGL_LIBRARIES}
    GLEW::GLEW
    glfw
)

# 3D Test executable
add_executable(3d_test 3D_test.cpp)
target_link_libraries(3d_test
    ${OPENGL_LIBRARIES}
    GLEW::GLEW
    glfw
)

# Build output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
