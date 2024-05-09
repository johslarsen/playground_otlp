#include "example.h"

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/provider.h>

// needs to be lazily initialized to happen after SetLoggerProvider
static auto& log() {
  // who knows if this should be thread-local?
  static auto log = opentelemetry::logs::Provider::GetLoggerProvider()->GetLogger("example_name", "example_lib");
  return log;
}

// needs to be lazily initialized to happen after SetTracerProvider
static auto& trace() {
  // who knows if this should be thread-local?
  static auto log = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("example_name");
  return log;
}

// needs to be lazily initialized to happen after SetMeterProvider
static auto& meter() {
  static auto meter = opentelemetry::metrics::Provider::GetMeterProvider()->GetMeter("example_name");
  return meter;
}

void example::foo(std::stop_token stop_token) {
  auto unrelated = trace()->StartSpan("other");
  unrelated->SetAttribute("bar", 42);
  opentelemetry::trace::Scope parent(trace()->StartSpan("parent_scope"));
  opentelemetry::trace::Scope child(trace()->StartSpan("child_scope"));
  log()->Info("implicitly tagged with child_scope");
  log()->Info("explicitly tagged with unrelated", unrelated->GetContext());

  opentelemetry::context::Context ctx;
  ctx = opentelemetry::trace::SetSpan(ctx, unrelated);

  std::map<std::string, std::string> attrs{{"label", "value"}};

  auto time_counter = meter()->CreateDoubleCounter("time_counter", "Time elapsed since start of example", "ms");
  auto uniform_histogram = meter()->CreateUInt64Histogram("uniform_histogram");
  for (size_t i = 0; !stop_token.stop_requested(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    time_counter->Add(1, attrs);

    uniform_histogram->Record(i % 10, attrs, ctx);
  }
}
