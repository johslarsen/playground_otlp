#include "example.h"

#include <opentelemetry/logs/provider.h>
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
  static auto log = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("example_name", "example_lib");
  return log;
}

void example::foo() {
  auto unrelated = trace()->StartSpan("other");
  unrelated->SetAttribute("bar", 42);
  opentelemetry::trace::Scope parent(trace()->StartSpan("parent_scope"));
  opentelemetry::trace::Scope child(trace()->StartSpan("child_scope"));
  log()->Info("implicitly tagged with child_scope");
  log()->Info("explicitly tagged with unrelated", unrelated->GetContext());
}
