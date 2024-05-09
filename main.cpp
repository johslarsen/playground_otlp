#include <opentelemetry/exporters/ostream/log_record_exporter.h>
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/provider.h>

#include "example.h"

int main() {
  using opentelemetry::exporter::trace::OStreamSpanExporter;
  using opentelemetry::sdk::trace::SimpleSpanProcessor;
  using opentelemetry::sdk::trace::TracerProviderFactory;
  opentelemetry::trace::Provider::SetTracerProvider(
      TracerProviderFactory::Create(
          std::make_unique<SimpleSpanProcessor>(
              std::make_unique<OStreamSpanExporter>())));

  using opentelemetry::exporter::logs::OStreamLogRecordExporter;
  using opentelemetry::sdk::logs::LoggerProviderFactory;
  using opentelemetry::sdk::logs::SimpleLogRecordProcessor;
  opentelemetry::logs::Provider::SetLoggerProvider(
      LoggerProviderFactory::Create(
          std::make_unique<SimpleLogRecordProcessor>(
              std::make_unique<OStreamLogRecordExporter>())));

  example::foo();
  return 0;
}
