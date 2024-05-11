#include <absl/log/initialize.h>
#include <absl/log/log_sink_registry.h>
#include <opentelemetry/exporters/ostream/metric_exporter.h>
#include <opentelemetry/exporters/prometheus/exporter.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/provider.h>

#include "example.h"
#include "logfmt_exporter.h"
#include "otlp_sink.h"

int main() {
  using opentelemetry::sdk::trace::SimpleSpanProcessor;
  using opentelemetry::sdk::trace::TracerProviderFactory;
  opentelemetry::trace::Provider::SetTracerProvider(
      TracerProviderFactory::Create(
          std::make_unique<SimpleSpanProcessor>(
              std::make_unique<LogfmtSpanExporter>())));

  using opentelemetry::sdk::logs::LoggerProviderFactory;
  using opentelemetry::sdk::logs::SimpleLogRecordProcessor;
  auto log_provider = LoggerProviderFactory::Create(
      std::make_unique<SimpleLogRecordProcessor>(
          std::make_unique<LogfmtExporter>()));

  absl::InitializeLog();
  OTLPSink sink(log_provider->GetLogger("absl"));
  absl::AddLogSink(&sink);

  opentelemetry::logs::Provider::SetLoggerProvider(std::move(log_provider));

  using opentelemetry::exporter::metrics::OStreamMetricExporter;
  using opentelemetry::exporter::metrics::PrometheusExporter;
  using opentelemetry::exporter::metrics::PrometheusExporterOptions;
  using opentelemetry::sdk::metrics::InstrumentType;
  using opentelemetry::sdk::metrics::PeriodicExportingMetricReader;
  using opentelemetry::sdk::metrics::PeriodicExportingMetricReaderOptions;
  auto p = std::make_unique<opentelemetry::sdk::metrics::MeterProvider>();
  p->AddMetricReader(std::make_unique<PeriodicExportingMetricReader>(
      std::make_unique<OStreamMetricExporter>(), PeriodicExportingMetricReaderOptions{
                                                     .export_interval_millis = std::chrono::seconds(1),
                                                     .export_timeout_millis = std::chrono::milliseconds(500),
                                                 }));
  PrometheusExporterOptions scraper_opts;  // localhost:9464 by default
  p->AddMetricReader(std::make_unique<PrometheusExporter>(scraper_opts));

  opentelemetry::metrics::Provider::SetMeterProvider(
      std::unique_ptr<opentelemetry::metrics::MeterProvider>(std::move(p)));

  // thread is only needed for metric exporters, because (end of) traces and logs are output live
  std::jthread app(example::foo);
  std::this_thread::sleep_for(std::chrono::seconds(10));
  return 0;
}
