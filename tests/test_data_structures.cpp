#define CATCH_CONFIG_RUNNER
#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"
#include "solakon_device.h"

using namespace solakon;

TEST_CASE("DeviceSnapshot: zero-initialization", "[data][snapshot]") {
    DeviceSnapshot snap{};
    REQUIRE(!snap.valid);
    REQUIRE(snap.info.model_name.empty());
    REQUIRE(snap.info.serial_number.empty());
    REQUIRE(snap.info.rated_power_kw == 0.0f);
    REQUIRE(snap.meter1.r_voltage == 0.0f);
    REQUIRE(snap.bms.soc == 0.0f);
    REQUIRE(snap.energy.pv_total_power_w == 0.0f);
}

TEST_CASE("DeviceSnapshot: valid flag", "[data][snapshot]") {
    DeviceSnapshot snap{};
    REQUIRE_FALSE(snap.valid);
    snap.valid = true;
    REQUIRE(snap.valid);
}

TEST_CASE("DeviceInfo: default values", "[data][info]") {
    DeviceInfo info{};
    REQUIRE(info.master_version == 0);
    REQUIRE(info.slave_version == 0);
    REQUIRE(info.model_id == 0);
    REQUIRE(info.num_strings == 0);
    REQUIRE(info.num_mppt == 0);
    REQUIRE(info.status1 == 0);
    REQUIRE(info.status3 == 0);
    REQUIRE(info.alarm1 == 0);
    REQUIRE(info.alarm2 == 0);
}

TEST_CASE("MeterData: default values", "[data][meter]") {
    MeterData m{};
    REQUIRE(!m.connected);
    REQUIRE(m.r_voltage == 0.0f);
    REQUIRE(m.s_voltage == 0.0f);
    REQUIRE(m.t_voltage == 0.0f);
    REQUIRE(m.r_current == 0.0f);
    REQUIRE(m.s_current == 0.0f);
    REQUIRE(m.t_current == 0.0f);
    REQUIRE(m.r_active_power == 0.0f);
    REQUIRE(m.s_active_power == 0.0f);
    REQUIRE(m.t_active_power == 0.0f);
    REQUIRE(m.combined_active_power == 0.0f);
    REQUIRE(m.frequency == 0.0f);
}

TEST_CASE("MeterData: connected flag", "[data][meter]") {
    MeterData m{};
    m.connected = true;
    REQUIRE(m.connected);
}

TEST_CASE("BMSData: default values", "[data][bms]") {
    BMSData bms{};
    REQUIRE(bms.online == 0);
    REQUIRE(bms.main_control == 0);
    REQUIRE(bms.sn.empty());
    REQUIRE(bms.slave_count == 0);
    REQUIRE(bms.voltage == 0.0f);
    REQUIRE(bms.current == 0.0f);
    REQUIRE(bms.temperature == 0.0f);
    REQUIRE(bms.soc == 0.0f);
    REQUIRE(bms.soh == 0.0f);
    REQUIRE(bms.remain_energy_wh == 0.0f);
}

TEST_CASE("BMSData: set values", "[data][bms]") {
    BMSData bms{};
    bms.online = 1;
    bms.soc = 85.0f;
    bms.voltage = 48.5f;
    bms.current = 25.0f;
    bms.temperature = 32.5f;
    bms.soh = 95.0f;
    bms.remain_energy_wh = 15000.0f;

    REQUIRE(bms.online == 1);
    REQUIRE(approx_equal(bms.soc, 85.0));
    REQUIRE(approx_equal(bms.voltage, 48.5));
    REQUIRE(approx_equal(bms.current, 25.0));
    REQUIRE(approx_equal(bms.temperature, 32.5));
    REQUIRE(approx_equal(bms.soh, 95.0));
    REQUIRE(approx_equal(bms.remain_energy_wh, 15000.0));
}

TEST_CASE("EnergyData: default values", "[data][energy]") {
    EnergyData e{};
    REQUIRE(e.pv_total_power_w == 0.0f);
    REQUIRE(e.total_charge_kwh == 0.0f);
    REQUIRE(e.total_charge_today_kwh == 0.0f);
    REQUIRE(e.total_discharge_kwh == 0.0f);
    REQUIRE(e.total_discharge_today_kwh == 0.0f);
    REQUIRE(e.total_feeder_kwh == 0.0f);
    REQUIRE(e.total_feeder_today_kwh == 0.0f);
    REQUIRE(e.total_consumption_kwh == 0.0f);
    REQUIRE(e.total_consumption_today_kwh == 0.0f);
    REQUIRE(e.total_output_kwh == 0.0f);
    REQUIRE(e.total_output_today_kwh == 0.0f);
    REQUIRE(e.total_load_kwh == 0.0f);
    REQUIRE(e.total_load_today_kwh == 0.0f);
}

TEST_CASE("EnergyData: set values", "[data][energy]") {
    EnergyData e{};
    e.pv_total_power_w = 5000.0f;
    e.total_charge_today_kwh = 12.5f;
    e.total_feeder_today_kwh = 8.3f;
    e.total_consumption_today_kwh = 15.0f;

    REQUIRE(approx_equal(e.pv_total_power_w, 5000.0));
    REQUIRE(approx_equal(e.total_charge_today_kwh, 12.5));
    REQUIRE(approx_equal(e.total_feeder_today_kwh, 8.3));
    REQUIRE(approx_equal(e.total_consumption_today_kwh, 15.0));
}

TEST_CASE("InverterStatus: enum values", "[data][enum]") {
    REQUIRE(static_cast<uint16_t>(InverterStatus::STANDBY) == 0);
    REQUIRE(static_cast<uint16_t>(InverterStatus::OPERATING) == 2);
    REQUIRE(static_cast<uint16_t>(InverterStatus::ERROR) == 6);
}

TEST_CASE("reg namespace: register constants are valid", "[data][reg]") {
    REQUIRE(reg::MODEL_NAME == 30000);
    REQUIRE(reg::SN == 30016);
    REQUIRE(reg::MASTER_VERSION == 36001);
    REQUIRE(reg::METER1_CONN == 38801);
    REQUIRE(reg::METER1_R_VOLTAGE == 38802);
    REQUIRE(reg::PROTOCOL_VERSION == 39000);
    REQUIRE(reg::PV_TOTAL_POWER == 39118);
    REQUIRE(reg::TOTAL_FEEDER == 39611);
}
