/**
 * @file AdapterTests.cpp
 * @brief Tests for the object, class and callback adapters.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <type_traits>

#include "patterns/Adapter.hpp"

using namespace CppVerseHub::Patterns;
using Catch::Approx;

TEST_CASE("Frame codec round-trips arbitrary text", "[adapter][codec]") {
    const std::string body = GENERATE(std::string(""), std::string("plain"), std::string("a|b|c"),
                                      std::string(R"(back\slash)"), std::string(R"(\|\\||)"));
    const Message m{"from|x", R"(to\y)", body, MessagePriority::Low};
    const auto decoded = decodeFrame(encodeFrame(m));
    REQUIRE(decoded.has_value());
    CHECK(*decoded == m);
}

TEST_CASE("Frame codec rejects malformed frames", "[adapter][codec]") {
    CHECK_FALSE(decodeFrame("").has_value());
    CHECK_FALSE(decodeFrame("a|b|1").has_value());     // missing body field
    CHECK_FALSE(decodeFrame("a|b|9|body").has_value());  // bad priority
    CHECK_FALSE(decodeFrame("a|b|1|x|y").has_value());   // unescaped separator in body
    CHECK_FALSE(decodeFrame("a|b|1|x\\").has_value());   // dangling escape
    CHECK(encodeFrame({"S", "R", "hi", MessagePriority::Urgent}) == "R|S|2|hi");
}

TEST_CASE("LegacyRadio reports C-style error codes", "[adapter][legacy]") {
    LegacyRadio radio;
    CHECK(radio.transmit(nullptr, 0) == LegacyRadio::kErrNull);
    const std::string big(LegacyRadio::kMaxFrame + 1, 'x');
    CHECK(radio.transmit(big.data(), big.size()) == LegacyRadio::kErrTooLong);
    CHECK(radio.transmit("abc", 3) == LegacyRadio::kOk);
    char small[2];
    CHECK(radio.poll(small, sizeof small) == LegacyRadio::kErrBufferSmall);
    CHECK(radio.pending() == 1);
    char buf[8];
    CHECK(radio.poll(buf, sizeof buf) == 3);
    CHECK(radio.poll(buf, sizeof buf) == 0);
}

TEST_CASE("RadioAdapter exposes the modern channel interface", "[adapter][object]") {
    LegacyRadio radio;
    RadioAdapter channel(radio, "ch-1");
    ICommunicationChannel& iface = channel;
    CHECK(iface.channelName() == "ch-1");
    const Message a{"Alpha", "Beta", "hello | world", MessagePriority::Urgent};
    const Message b{"Beta", "Alpha", "ack", MessagePriority::Normal};
    CHECK(iface.send(a));
    CHECK(iface.send(b));
    CHECK(iface.receive() == a);
    CHECK(iface.receive() == b);
    CHECK_FALSE(iface.receive().has_value());
}

TEST_CASE("RadioAdapter rejects oversized messages and skips garbage frames", "[adapter][object]") {
    LegacyRadio radio;
    RadioAdapter channel(radio, "ch");
    CHECK_FALSE(channel.send({"a", "b", std::string(400, 'z'), MessagePriority::Low}));
    CHECK(radio.transmit("garbage", 7) == LegacyRadio::kOk);  // another legacy client
    channel.send({"a", "b", "ok", MessagePriority::Low});
    const auto m = channel.receive();
    REQUIRE(m.has_value());
    CHECK(m->body == "ok");
    CHECK(channel.malformedFrames() == 1);
}

TEST_CASE("ThermalSensorAdapter converts centi-kelvin", "[adapter][class]") {
    ThermalSensorAdapter sensor(27315);
    const ITemperatureSensor& iface = sensor;
    CHECK(iface.celsius() == Approx(0.0).margin(1e-9));
    CHECK(iface.fahrenheit() == Approx(32.0));
    sensor.calibrate(100.0);
    CHECK(sensor.readCentiKelvin() == 37315);
    CHECK(iface.fahrenheit() == Approx(212.0));
    sensor.calibrate(-40.0);
    CHECK(iface.celsius() == Approx(iface.fahrenheit()));
}

TEST_CASE("Class adapter does not publicly expose the adaptee", "[adapter][class]") {
    STATIC_REQUIRE(std::is_base_of_v<ITemperatureSensor, ThermalSensorAdapter>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<ThermalSensorAdapter*, LegacyThermalSensor*>);
}

TEST_CASE("CallbackBridge forwards C callbacks to std::function and unregisters (RAII)", "[adapter][callback]") {
    LegacyEventPump pump;
    std::vector<int> a;
    std::vector<int> b;
    {
        CallbackBridge first(pump, [&](int c) { a.push_back(c); });
        CallbackBridge second(pump, [&](int c) { b.push_back(c * 2); });
        CHECK(first.id() != second.id());
        CHECK(pump.size() == 2);
        CHECK(pump.fire(3) == 2);
    }
    CHECK(pump.size() == 0);
    CHECK(pump.fire(4) == 0);
    CHECK(a == std::vector<int>{3});
    CHECK(b == std::vector<int>{6});
    CHECK_THROWS_AS(CallbackBridge(pump, nullptr), std::invalid_argument);
}

TEST_CASE("LegacyEventPump validates registrations", "[adapter][legacy]") {
    LegacyEventPump pump;
    CHECK(pump.registerCallback(nullptr, nullptr) == 0);
    CHECK_FALSE(pump.unregisterCallback(42));
}
