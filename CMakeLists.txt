cmake_minimum_required(VERSION 3.20.0)

project(
  otlp
  VERSION 0.1.0
  LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(external/abseil-cpp)

set(WITH_STL CXX20 CACHE BOOL "")
set(WITH_PROMETHEUS ON CACHE BOOL "")
set(BUILD_TESTING OFF CACHE BOOL "")
set(WITH_BENCHMARK OFF CACHE BOOL "")

add_subdirectory(external/opentelemetry-cpp)

add_library(example example.h example.cpp)
target_link_libraries(example opentelemetry_api absl_log)

add_executable(main main.cpp otlp_sink.h)
target_link_libraries(
  main
  example
  opentelemetry_sdk
  opentelemetry_exporter_ostream_logs
  opentelemetry_exporter_ostream_metrics
  opentelemetry_exporter_ostream_span
  opentelemetry_exporter_prometheus
  absl_log_initialize
  absl_log_sink)
