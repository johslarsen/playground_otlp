#pragma once

#include <opentelemetry/sdk/logs/exporter.h>
#include <opentelemetry/sdk/logs/read_write_log_record.h>
#include <opentelemetry/sdk/metrics/export/metric_producer.h>
#include <opentelemetry/sdk/metrics/push_metric_exporter.h>
#include <opentelemetry/sdk/trace/exporter.h>
#include <opentelemetry/sdk/trace/span_data.h>

#include <format>
#include <iostream>

struct AttrStreamer {
  std::ostream& out;

  void operator()(std::string v) { out << std::quoted(v); }
  void operator()(std::string_view v) { out << std::quoted(v); }
  void operator()(std::integral auto v) { out << v; }
  void operator()(double v) { out << v; }
  void operator()(bool v) { out << (v ? "true" : "false"); }
  void operator()(std::ranges::range auto values) {
    out << "[";
    std::string_view separator = "";
    for (const auto& v : values) {
      out << std::exchange(separator, ",");
      (*this)(v);
    }
    out << "]";
  }
};

template <typename T>
struct AttrKV {
  std::string_view key;
  T value;
};
template <typename T>
inline std::ostream& operator<<(std::ostream& out, const AttrKV<T>& kv) {
  out << kv.key << "=";
  std::visit(AttrStreamer{out}, kv.value);
  return out;
}

template <typename ID>
struct NamedID {
  std::string_view key;
  const ID& id;
};
template <typename ID>
inline std::ostream& operator<<(std::ostream& out, const NamedID<ID>& nid) {
  std::array<char, 2 * ID::kSize> hex;  // NOLINT(*-init), initialized during ToLowerBase16
  nid.id.ToLowerBase16(std::span(hex));
  return out << nid.key << "=" << std::string_view(hex.begin(), hex.end());
}

class LogfmtExporter : public opentelemetry::sdk::logs::LogRecordExporter {
  std::ostream& _out;
  bool _shutdown = false;

 public:
  explicit LogfmtExporter(std::ostream& out = std::cout) noexcept : _out(out) {}

  std::unique_ptr<opentelemetry::sdk::logs::Recordable> MakeRecordable() noexcept override {
    return std::make_unique<opentelemetry::sdk::logs::ReadWriteLogRecord>();
  }

  opentelemetry::sdk::common::ExportResult Export(const std::span<std::unique_ptr<opentelemetry::sdk::logs::Recordable>>& records) noexcept
      override {
    if (_shutdown) return opentelemetry::sdk::common::ExportResult::kFailure;

    for (const auto& record : records) {
      if (record == nullptr) continue;
      const auto* r = dynamic_cast<opentelemetry::sdk::logs::ReadWriteLogRecord*>(record.get());

      _out << "level=" << r->GetSeverityText()
           << ' ' << AttrKV{"msg", r->GetBody()};
      for (const auto& [k, v] : r->GetAttributes()) {
        _out << ' ' << AttrKV{k, v};
      }
      if (const auto& id = r->GetTraceId(); id.IsValid()) _out << ' ' << NamedID{"trace_id", id};
      if (const auto& id = r->GetSpanId(); id.IsValid()) _out << ' ' << NamedID{"span_id", id};
      if (auto id = r->GetEventId(); id != 0) _out << " event_id=" << id;
      if (auto e = r->GetEventName(); !e.empty()) _out << " event_id=" << std::quoted(e);
      auto t = r->GetSpanId();
      _out << "\n";
    }

    return opentelemetry::sdk::common::ExportResult::kSuccess;
  }

  bool ForceFlush(std::chrono::microseconds /*timeout*/ = std::chrono::microseconds::max()) noexcept override {
    _out.flush();
    return true;
  }

  bool Shutdown(std::chrono::microseconds /*timeout*/ = std::chrono::microseconds::max()) noexcept override {
    return (_shutdown = true);
  }
};

class LogfmtSpanExporter : public opentelemetry::sdk::trace::SpanExporter {
  std::ostream& _out;
  bool _shutdown = false;

 public:
  explicit LogfmtSpanExporter(std::ostream& out = std::cout) noexcept : _out(out) {}

  std::unique_ptr<opentelemetry::sdk::trace::Recordable> MakeRecordable() noexcept override {
    return std::make_unique<opentelemetry::sdk::trace::SpanData>();
  }

  opentelemetry::sdk::common::ExportResult Export(const std::span<std::unique_ptr<opentelemetry::sdk::trace::Recordable>>& records) noexcept
      override {
    if (_shutdown) return opentelemetry::sdk::common::ExportResult::kFailure;

    for (const auto& record : records) {
      if (record == nullptr) continue;
      const auto* s = dynamic_cast<opentelemetry::sdk::trace::SpanData*>(record.get());

      _out << "level=" << (s->GetParentSpanId().IsValid() ? "DEBUG" : "INFO")
           << " name=" << std::quoted(s->GetName());
      for (const auto& [k, v] : s->GetAttributes()) {
        _out << ' ' << AttrKV{k, v};
      }
      _out << " start=" << std::format("{:%FT%T}", static_cast<std::chrono::system_clock::time_point>(s->GetStartTime()));
      _out << " duration=" << s->GetDuration();
      if (auto d = s->GetDescription(); !d.empty()) _out << " description=" << std::quoted(d);
      if (const auto& id = s->GetTraceId(); id.IsValid()) _out << ' ' << NamedID{"trace_id", id};
      if (const auto& id = s->GetSpanId(); id.IsValid()) _out << ' ' << NamedID{"span_id", id};
      if (const auto& id = s->GetParentSpanId(); id.IsValid()) _out << ' ' << NamedID{"parent_span_id", id};

      _out << "\n";
    }

    return opentelemetry::sdk::common::ExportResult::kSuccess;
  }

  bool ForceFlush(std::chrono::microseconds /*timeout*/ = std::chrono::microseconds::max()) noexcept override {
    _out.flush();
    return true;
  }

  bool Shutdown(std::chrono::microseconds /*timeout*/ = std::chrono::microseconds::max()) noexcept override {
    return (_shutdown = true);
  }
};

struct MetricStreamer {
  std::ostream& out;

  void operator()(const opentelemetry::sdk::metrics::DropPointData& p) {}
  void operator()(const opentelemetry::sdk::metrics::SumPointData& p) {
    std::visit(AttrStreamer{out << " value="}, p.value_);
  }
  void operator()(const opentelemetry::sdk::metrics::LastValuePointData& p) {
    std::visit(AttrStreamer{out << " value="}, p.value_);
  }
  void operator()(const opentelemetry::sdk::metrics::HistogramPointData& p) {
    AttrStreamer{out << " counts="}(p.counts_);
    AttrStreamer{out << " buckets="}(p.boundaries_);
  }
};

class LogfmtMetricExporter : public opentelemetry::sdk::metrics::PushMetricExporter {
  std::ostream& _out;
  opentelemetry::sdk::metrics::AggregationTemporality _aggregation_temporality;
  bool _shutdown = false;

 public:
  explicit LogfmtMetricExporter(
      std::ostream& out = std::cout,
      opentelemetry::sdk::metrics::AggregationTemporality aggregation_temporality = opentelemetry::sdk::metrics::AggregationTemporality::kCumulative) noexcept
      : _out(out), _aggregation_temporality(aggregation_temporality) {}

  opentelemetry::sdk::common::ExportResult Export(const opentelemetry::sdk::metrics::ResourceMetrics& data) noexcept override {
    if (_shutdown) return opentelemetry::sdk::common::ExportResult::kFailure;

    for (const auto& meter : data.scope_metric_data_) {
      // WARN: the OStreamMetricExporter took some lock here to guard "_out",
      // indicating that this is probably used multi-threaded. however, if that
      // is the case it does not help much because _out is usually std::cout,
      // so locking it within here does not block other std::cout usage!

      for (const auto& instrument : meter.metric_data_) {
        for (const auto& p : instrument.point_data_attr_) {
          _out << "level=INFO"
               << " meter=" << std::quoted(meter.scope_->GetName())
               << " instrument=" << std::quoted(instrument.instrument_descriptor.name_);
          for (const auto& [k, v] : p.attributes) {
            _out << ' ' << AttrKV{k, v};
          }
          std::visit(MetricStreamer{_out}, p.point_data);
          if (const auto& u = instrument.instrument_descriptor.unit_; !std::empty(u)) _out << " unit=" << u;
          _out << "\n";
        }
      }
    }

    return opentelemetry::sdk::common::ExportResult::kSuccess;
  }

  opentelemetry::sdk::metrics::AggregationTemporality GetAggregationTemporality(
      opentelemetry::sdk::metrics::InstrumentType /*instrument_type*/) const noexcept override {
    return _aggregation_temporality;
  }

  bool ForceFlush(std::chrono::microseconds /*timeout*/ = std::chrono::microseconds::max()) noexcept override {
    _out.flush();
    return true;
  }

  bool Shutdown(std::chrono::microseconds /*timeout*/ = std::chrono::microseconds::max()) noexcept override {
    return (_shutdown = true);
  }
};
