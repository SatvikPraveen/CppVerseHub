/**
 * @file Adapter.hpp
 * @brief Adapter pattern: making legacy space-station subsystems fit modern interfaces.
 *
 * Three adapter flavours are shown, each solving a different integration problem:
 *  - **Object adapter** (`RadioAdapter`): wraps (composes) a `LegacyRadio` that speaks a C-style
 *    API of `const char*` frames and integer status codes, and exposes the typed, exception-safe
 *    `ICommunicationChannel`. Includes a reversible wire encoding with escaping.
 *  - **Class adapter** (`ThermalSensorAdapter`): inherits the legacy implementation *privately*
 *    (implementation inheritance, not an is-a relationship) and the target interface publicly,
 *    converting centi-kelvin integers into degrees Celsius / Fahrenheit.
 *  - **Callback adapter** (`CallbackBridge`): adapts a modern `std::function` handler to a legacy
 *    C callback registry that only accepts `void (*)(int, void*)` plus a user-data pointer — the
 *    standard trampoline technique. The bridge is RAII: it unregisters itself on destruction.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace CppVerseHub::Patterns {

// ============================================================================
// Object adapter: legacy radio -> modern communication channel
// ============================================================================

/// @brief Message priority.
enum class MessagePriority : std::uint8_t { Low = 0, Normal = 1, Urgent = 2 };

/**
 * @brief Modern, typed message.
 */
struct Message {
    std::string sender;                                 ///< Sender id.
    std::string recipient;                              ///< Recipient id.
    std::string body;                                   ///< Payload (any characters).
    MessagePriority priority = MessagePriority::Normal; ///< Priority.

    /// @brief Member-wise equality. @return true if equal.
    friend bool operator==(const Message&, const Message&) = default;
};

/**
 * @brief Target interface expected by modern client code.
 */
class ICommunicationChannel {
public:
    virtual ~ICommunicationChannel() = default;
    /// @brief Send a message. @param message Message. @return true if accepted for transmission.
    virtual bool send(const Message& message) = 0;
    /// @brief Receive the next message, if any. @return Message or nullopt.
    [[nodiscard]] virtual std::optional<Message> receive() = 0;
    /// @brief Channel name. @return Name.
    [[nodiscard]] virtual std::string channelName() const = 0;

protected:
    ICommunicationChannel() = default;
    ICommunicationChannel(const ICommunicationChannel&) = default;
    ICommunicationChannel& operator=(const ICommunicationChannel&) = default;
    ICommunicationChannel(ICommunicationChannel&&) = default;
    ICommunicationChannel& operator=(ICommunicationChannel&&) = default;
};

/**
 * @brief Adaptee: a legacy loop-back radio with a C-style API.
 *
 * Frames are raw byte strings of at most `kMaxFrame` bytes. `transmit` returns 0 on success and a
 * negative error code otherwise; `poll` copies the oldest frame into a caller buffer.
 */
class LegacyRadio {
public:
    static constexpr std::size_t kMaxFrame = 256; ///< Maximum frame size in bytes.
    static constexpr int kOk = 0;                 ///< Success.
    static constexpr int kErrNull = -1;           ///< Null frame pointer.
    static constexpr int kErrTooLong = -2;        ///< Frame exceeds kMaxFrame.
    static constexpr int kErrBufferSmall = -3;    ///< Receive buffer too small.

    /**
     * @brief Queue a frame for (loop-back) delivery.
     * @param frame Frame bytes.
     * @param length Frame length.
     * @return kOk or a negative error code.
     */
    int transmit(const char* frame, std::size_t length);

    /**
     * @brief Copy the oldest frame into @p buffer.
     * @param buffer Destination.
     * @param capacity Destination size.
     * @return Bytes written (0 if no frame pending) or kErrBufferSmall.
     */
    int poll(char* buffer, std::size_t capacity);

    /// @brief Frames waiting to be polled. @return Count.
    [[nodiscard]] std::size_t pending() const noexcept { return frames_.size(); }

private:
    std::deque<std::string> frames_;
};

/**
 * @brief Encode a message as a legacy frame: `TO|FROM|P|BODY` with `\` escaping of `|` and `\`.
 * @param message Message.
 * @return Encoded frame.
 */
[[nodiscard]] std::string encodeFrame(const Message& message);

/**
 * @brief Decode a legacy frame produced by encodeFrame.
 * @param frame Frame text.
 * @return Message, or nullopt for malformed frames.
 */
[[nodiscard]] std::optional<Message> decodeFrame(std::string_view frame);

/**
 * @brief Object adapter: implements ICommunicationChannel on top of a LegacyRadio.
 */
class RadioAdapter final : public ICommunicationChannel {
public:
    /**
     * @brief Construct.
     * @param radio Adaptee (not owned; must outlive the adapter).
     * @param name Channel name.
     */
    RadioAdapter(LegacyRadio& radio, std::string name) : radio_(&radio), name_(std::move(name)) {}

    bool send(const Message& message) override;
    [[nodiscard]] std::optional<Message> receive() override;
    [[nodiscard]] std::string channelName() const override { return name_; }
    /// @brief Frames dropped because they could not be decoded. @return Count.
    [[nodiscard]] std::size_t malformedFrames() const noexcept { return malformed_; }

private:
    LegacyRadio* radio_;
    std::string name_;
    std::size_t malformed_ = 0;
};

// ============================================================================
// Class adapter: legacy thermal sensor -> modern temperature sensor
// ============================================================================

/**
 * @brief Target interface for temperature sensors.
 */
class ITemperatureSensor {
public:
    virtual ~ITemperatureSensor() = default;
    /// @brief Temperature in degrees Celsius. @return Celsius.
    [[nodiscard]] virtual double celsius() const = 0;
    /// @brief Temperature in degrees Fahrenheit. @return Fahrenheit.
    [[nodiscard]] double fahrenheit() const { return celsius() * 9.0 / 5.0 + 32.0; }

protected:
    ITemperatureSensor() = default;
    ITemperatureSensor(const ITemperatureSensor&) = default;
    ITemperatureSensor& operator=(const ITemperatureSensor&) = default;
    ITemperatureSensor(ITemperatureSensor&&) = default;
    ITemperatureSensor& operator=(ITemperatureSensor&&) = default;
};

/**
 * @brief Adaptee: reports temperature as an integer number of centi-kelvin.
 */
class LegacyThermalSensor {
public:
    /// @brief Construct. @param centiKelvin Initial reading.
    explicit LegacyThermalSensor(std::int32_t centiKelvin = 29315) noexcept : reading_(centiKelvin) {}
    /// @brief Raw reading. @return Centi-kelvin.
    [[nodiscard]] std::int32_t readCentiKelvin() const noexcept { return reading_; }
    /// @brief Set the raw reading. @param centiKelvin New reading.
    void setCentiKelvin(std::int32_t centiKelvin) noexcept { reading_ = centiKelvin; }

private:
    std::int32_t reading_;
};

/**
 * @brief Class adapter: public target interface, private adaptee implementation.
 */
class ThermalSensorAdapter final
    : public ITemperatureSensor
    , private LegacyThermalSensor {
public:
    /// @brief Construct. @param centiKelvin Initial raw reading.
    explicit ThermalSensorAdapter(std::int32_t centiKelvin = 29315) noexcept
        : LegacyThermalSensor(centiKelvin) {}
    [[nodiscard]] double celsius() const override;
    /// @brief Calibrate the sensor to a Celsius value (rounded to 0.01 K). @param celsius Value.
    void calibrate(double celsius) noexcept;
    using LegacyThermalSensor::readCentiKelvin; // selectively re-expose part of the adaptee
};

// ============================================================================
// Callback adapter: std::function -> C function pointer + void*
// ============================================================================

/**
 * @brief Adaptee: legacy event pump accepting only C callbacks.
 */
class LegacyEventPump {
public:
    /// @brief C callback signature.
    using Callback = void (*)(int code, void* userData);

    /**
     * @brief Register a callback.
     * @param cb Callback (non-null).
     * @param userData Opaque pointer passed back to @p cb.
     * @return Registration id (> 0), or 0 if @p cb is null.
     */
    int registerCallback(Callback cb, void* userData);
    /// @brief Remove a registration. @param id Registration id. @return true if it existed.
    bool unregisterCallback(int id);
    /// @brief Deliver @p code to every registered callback. @param code Event code. @return Callbacks
    /// invoked.
    std::size_t fire(int code);
    /// @brief Registered callbacks. @return Count.
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }

private:
    struct Entry {
        int id;
        Callback cb;
        void* user;
    };
    std::vector<Entry> entries_;
    int nextId_ = 1;
};

/**
 * @brief RAII bridge that lets a `std::function<void(int)>` receive LegacyEventPump events.
 *
 * Non-copyable and non-movable because the pump stores `this` as user data.
 */
class CallbackBridge {
public:
    /**
     * @brief Register @p handler with @p pump.
     * @param pump Adaptee (must outlive the bridge).
     * @param handler Modern handler (non-empty).
     */
    CallbackBridge(LegacyEventPump& pump, std::function<void(int)> handler);
    CallbackBridge(const CallbackBridge&) = delete;
    CallbackBridge& operator=(const CallbackBridge&) = delete;
    CallbackBridge(CallbackBridge&&) = delete;
    CallbackBridge& operator=(CallbackBridge&&) = delete;
    /// @brief Unregisters from the pump.
    ~CallbackBridge();

    /// @brief Registration id. @return Id.
    [[nodiscard]] int id() const noexcept { return id_; }

private:
    static void trampoline(int code, void* self);

    LegacyEventPump* pump_;
    std::function<void(int)> handler_;
    int id_ = 0;
};

/**
 * @brief Showcase the three adapters.
 * @param out Stream receiving the narration.
 */
void demonstrateAdapter(std::ostream& out = std::cout);

} // namespace CppVerseHub::Patterns
