cmake_minimum_required(VERSION 3.2.0)

project(
  otlp
  VERSION 0.1.0
  LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/opentelemetry-cpp)

add_library(example example.cpp)
target_link_libraries(example opentelemetry_api)

add_executable(main main.cpp)
target_link_libraries(main example opentelemetry_sdk opentelemetry_exporter_ostream_logs)