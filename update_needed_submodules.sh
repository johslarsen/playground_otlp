#!/bin/sh -
git submodule update --init external/opentelemetry-cpp/
(cd external/opentelemetry-cpp/; git submodule update --init third_party/prometheus-cpp/)
(cd external/opentelemetry-cpp/third_party/prometheus-cpp/; git submodule update --init 3rdparty/civetweb/)
