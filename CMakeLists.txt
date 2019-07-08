cmake_minimum_required(VERSION 3.0.0)
project(Lispy VERSION 0.0.5)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

configure_file(
    "${PROJECT_SOURCE_DIR}/lispy_config.h.in"
    "${PROJECT_BINARY_DIR}/lispy_config.h")

include_directories("${PROJECT_BINARY_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/mpc")
add_subdirectory(mpc)

add_executable(lispy main.cpp lval.cpp lval_error.cpp builtin.cpp)
target_link_libraries(lispy edit MPC)
