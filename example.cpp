#include "example.h"

#include <opentelemetry/logs/provider.h>

// needs to be lazily initialized to happen after SetLoggerProvider
static auto& log() {
  // who knows if this should be thread-local?
  static auto log = opentelemetry::logs::Provider::GetLoggerProvider()->GetLogger("example_name", "example_lib");
  return log;
}

void example::foo() {
  log()->Info("body");
}
