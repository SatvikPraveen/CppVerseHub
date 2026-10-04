/**
 * @file Adapter.cpp
 * @brief Legacy subsystems and their adapters.
 */

#include "patterns/Adapter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace CppVerseHub::Patterns {

// ---------------------------------------------------------------------------
// LegacyRadio
// ---------------------------------------------------------------------------

int LegacyRadio::transmit(const char* frame, std::size_t length) {
    if (frame == nullptr) {
        return kErrNull;
    }
    if (length > kMaxFrame) {
        return kErrTooLong;
    }
    frames_.emplace_back(frame, length);
    return kOk;
}

int LegacyRadio::poll(char* buffer, std::size_t capacity) {
    if (frames_.empty()) {
        return 0;
    }
    const std::string& front = frames_.front();
    if (buffer == nullptr || capacity < front.size()) {
        return kErrBufferSmall;
    }
    std::copy(front.begin(), front.end(), buffer);
    const auto written = static_cast<int>(front.size());
    frames_.pop_front();
    return written;
}

// ---------------------------------------------------------------------------
// Frame codec
// ---------------------------------------------------------------------------

namespace {
void appendEscaped(std::string& out, std::string_view field) {
    for (const char c : field) {
        if (c == '|' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
}
}  // namespace

std::string encodeFrame(const Message& message) {
    std::string frame;
    frame.reserve(message.sender.size() + message.recipient.size() + message.body.size() + 8);
    appendEscaped(frame, message.recipient);
    frame.push_back('|');
    appendEscaped(frame, message.sender);
    frame.push_back('|');
    frame.push_back(static_cast<char>('0' + static_cast<int>(message.priority)));
    frame.push_back('|');
    appendEscaped(frame, message.body);
    return frame;
}

std::optional<Message> decodeFrame(std::string_view frame) {
    std::array<std::string, 4> fields;
    std::size_t field = 0;
    for (std::size_t i = 0; i < frame.size(); ++i) {
        const char c = frame[i];
        if (c == '\\') {
            if (i + 1 >= frame.size()) {
                return std::nullopt;  // dangling escape
            }
            fields[field].push_back(frame[++i]);
        } else if (c == '|' && field < 3) {
            ++field;
        } else if (c == '|') {
            return std::nullopt;  // unescaped separator inside the body
        } else {
            fields[field].push_back(c);
        }
    }
    if (field != 3 || fields[2].size() != 1 || fields[2][0] < '0' || fields[2][0] > '2') {
        return std::nullopt;
    }
    Message m;
    m.recipient = std::move(fields[0]);
    m.sender = std::move(fields[1]);
    m.priority = static_cast<MessagePriority>(fields[2][0] - '0');
    m.body = std::move(fields[3]);
    return m;
}

// ---------------------------------------------------------------------------
// RadioAdapter
// ---------------------------------------------------------------------------

bool RadioAdapter::send(const Message& message) {
    const std::string frame = encodeFrame(message);
    return radio_->transmit(frame.data(), frame.size()) == LegacyRadio::kOk;
}

std::optional<Message> RadioAdapter::receive() {
    std::array<char, LegacyRadio::kMaxFrame> buffer{};
    while (true) {
        const int n = radio_->poll(buffer.data(), buffer.size());
        if (n <= 0) {
            return std::nullopt;  // nothing pending (buffer is always large enough)
        }
        if (auto message = decodeFrame(std::string_view(buffer.data(), static_cast<std::size_t>(n)))) {
            return message;
        }
        ++malformed_;  // skip garbage frames written by other legacy clients
    }
}

// ---------------------------------------------------------------------------
// ThermalSensorAdapter
// ---------------------------------------------------------------------------

double ThermalSensorAdapter::celsius() const { return static_cast<double>(readCentiKelvin()) / 100.0 - 273.15; }

void ThermalSensorAdapter::calibrate(double celsius) noexcept {
    setCentiKelvin(static_cast<std::int32_t>(std::lround((celsius + 273.15) * 100.0)));
}

// ---------------------------------------------------------------------------
// LegacyEventPump / CallbackBridge
// ---------------------------------------------------------------------------

int LegacyEventPump::registerCallback(Callback cb, void* userData) {
    if (cb == nullptr) {
        return 0;
    }
    const int id = nextId_++;
    entries_.push_back({id, cb, userData});
    return id;
}

bool LegacyEventPump::unregisterCallback(int id) {
    const auto before = entries_.size();
    std::erase_if(entries_, [id](const Entry& e) { return e.id == id; });
    return entries_.size() != before;
}

std::size_t LegacyEventPump::fire(int code) {
    const auto snapshot = entries_;  // callbacks may unregister themselves
    for (const auto& e : snapshot) {
        e.cb(code, e.user);
    }
    return snapshot.size();
}

CallbackBridge::CallbackBridge(LegacyEventPump& pump, std::function<void(int)> handler)
    : pump_(&pump), handler_(std::move(handler)) {
    if (!handler_) {
        throw std::invalid_argument("CallbackBridge requires a handler");
    }
    id_ = pump_->registerCallback(&CallbackBridge::trampoline, this);
}

CallbackBridge::~CallbackBridge() { (void)pump_->unregisterCallback(id_); }

void CallbackBridge::trampoline(int code, void* self) { static_cast<CallbackBridge*>(self)->handler_(code); }

// ---------------------------------------------------------------------------
// Showcase
// ---------------------------------------------------------------------------

void demonstrateAdapter(std::ostream& out) {
    out << "=== Adapter pattern ===\n";
    LegacyRadio radio;
    RadioAdapter channel(radio, "subspace-1");
    const Message original{"Station Alpha", "Fleet|Command", R"(Status: all \ systems | nominal)",
                           MessagePriority::Urgent};
    (void)channel.send(original);
    out << "  legacy frame: " << encodeFrame(original) << '\n';
    const auto received = channel.receive();
    out << "  round-trip intact: " << std::boolalpha << (received && *received == original) << '\n';

    ThermalSensorAdapter sensor(37315);
    out << "  thermal sensor: " << sensor.readCentiKelvin() << " cK = " << sensor.celsius() << " C = "
        << sensor.fahrenheit() << " F\n";

    LegacyEventPump pump;
    std::vector<int> codes;
    {
        CallbackBridge bridge(pump, [&codes](int code) { codes.push_back(code); });
        (void)pump.fire(7);
        (void)pump.fire(42);
    }
    (void)pump.fire(99);  // bridge gone: not delivered
    out << "  callback bridge received " << codes.size() << " events; pump registrations now " << pump.size()
        << '\n';
}

}  // namespace CppVerseHub::Patterns
