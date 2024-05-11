#pragma once

#include <absl/container/inlined_vector.h>
#include <absl/log/log_sink.h>
#include <opentelemetry/logs/provider.h>

class OTLPSink : public absl::LogSink {
  opentelemetry::nostd::shared_ptr<opentelemetry::logs::Logger> _logger;

 public:
  explicit OTLPSink(opentelemetry::nostd::shared_ptr<opentelemetry::logs::Logger> logger) : _logger(std::move(logger)) {}

  ~OTLPSink() override = default;

  void Send(const absl::LogEntry& entry) override {
    absl::InlinedVector<std::pair<std::string_view, opentelemetry::common::AttributeValue>, 5> attr;
    attr.emplace_back("basename", entry.source_basename());
    attr.emplace_back("line", entry.source_line());
    attr.emplace_back("thread_id", entry.tid());
    if (!entry.stacktrace().empty()) attr.emplace_back("stacktrace", entry.stacktrace());
    if (entry.verbosity() != absl::LogEntry::kNoVerbosityLevel) attr.emplace_back("verbosity", entry.verbosity());

    _logger->EmitLogRecord(entry.text_message(), as_otlp(entry.log_severity()), absl::ToChronoTime(entry.timestamp()), std::move(attr));
  }

  OTLPSink(const OTLPSink&) = default;
  OTLPSink& operator=(const OTLPSink&) = default;
  OTLPSink(OTLPSink&&) = default;
  OTLPSink& operator=(OTLPSink&&) = default;

 private:
  opentelemetry::logs::Severity as_otlp(absl::LogSeverity severity) {
    switch (severity) {
      case absl::LogSeverity::kInfo:
        return opentelemetry::logs::Severity::kInfo;
      case absl::LogSeverity::kWarning:
        return opentelemetry::logs::Severity::kWarn;
      case absl::LogSeverity::kError:
        return opentelemetry::logs::Severity::kError;
      case absl::LogSeverity::kFatal:
        return opentelemetry::logs::Severity::kFatal;
    }
    abort();  // unreachable
  }
};
