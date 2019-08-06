cmake_minimum_required(VERSION 3.0.0)
project(Lispy VERSION 0.3.0)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

configure_file(
    "${PROJECT_SOURCE_DIR}/lispy_config.h.in"
    "${PROJECT_BINARY_DIR}/lispy_config.h")

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/mpc")
include_directories("${PROJECT_SOURCE_DIR}/tclap/include")
add_subdirectory(mpc)

add_executable(gen_headers gen_headers.cpp)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated.hpp
  COMMAND gen_headers ${CMAKE_CURRENT_BINARY_DIR}/generated.hpp
  DEPENDS gen_headers language.txt prelude.lspy
  )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(lispy main.cpp lispy.cpp lval.cpp lval_error.cpp builtin.cpp lenv.cpp ${CMAKE_CURRENT_BINARY_DIR}/generated.hpp)
target_link_libraries(lispy linenoise MPC)

install(TARGETS lispy)
