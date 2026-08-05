#pragma once
/**
 * @file solakon_device.h
 * @brief Solakon ONE Modbus device data structures and device abstraction.
 *
 * Defines register addresses, data structures, and the SolakonDevice class
 * for reading a complete snapshot from a Solakon ONE hybrid inverter via
 * Modbus TCP. Register addresses and data types are based on
 * *Solakon ONE Modbus Protocol v02/26*.
 *
 * Usage:
 * @code
 *   solakon::SolakonDevice device;
 *   device.connect("192.168.178.121", 502);
 *
 *   auto snap = device.fetch_snapshot();
 *   if (snap.valid) {
 *       printf("PV power: %.1f W\n", snap.energy.pv_total_power_w);
 *       printf("AC power: %.1f W\n", snap.info.ac_active_power);
 *   }
 *
 *   device.disconnect();
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace solakon {

/**
 * @brief Register address constants from Solakon ONE Modbus Protocol v02/26.
 */
namespace reg {

    // Inverter info (Table 2-1)
    constexpr uint16_t MODEL_NAME = 30000;       // STR, 16 registers
    constexpr uint16_t SN = 30016;                // STR, 16 registers
    constexpr uint16_t MFG_ID = 30032;            // STR, 16 registers

    // Inverter version (Table 2-2)
    constexpr uint16_t MASTER_VERSION = 36001;    // U16
    constexpr uint16_t SLAVE_VERSION = 36002;     // U16
    constexpr uint16_t MANAGER_VERSION = 36003;   // U16

    // Meter1 info (Table 2-2)
    constexpr uint16_t METER1_SN = 36100;         // STR, 16 registers
    constexpr uint16_t METER1_MFG = 36116;        // STR, 16 registers
    constexpr uint16_t METER1_TYPE = 36132;       // STR, 16 registers
    constexpr uint16_t METER1_VERSION = 36148;    // U16

    // Meter2 info
    constexpr uint16_t METER2_SN = 36200;         // STR, 16 registers
    constexpr uint16_t METER2_MFG = 36216;        // STR, 16 registers
    constexpr uint16_t METER2_TYPE = 36232;       // STR, 16 registers
    constexpr uint16_t METER2_VERSION = 36248;    // U16

    // BMS1 info (Table 2-3)
    constexpr uint16_t BMS1 = 37002;              // U16
    constexpr uint16_t BMS1_MAIN = 37003;         // U16
    constexpr uint16_t BMS1_SN = 37005;           // STR, 16 registers
    constexpr uint16_t BMS1_SLAVE_COUNT = 37032;  // U16
    constexpr uint16_t BMS1_SLAVE1_VER = 37033;   // U16
    constexpr uint16_t BMS1_SLAVE2_VER = 37034;   // U16
    constexpr uint16_t BMS1_VOLTAGE = 37609;      // U16, V/10
    constexpr uint16_t BMS1_CURRENT = 37610;      // I16, A/10
    constexpr uint16_t BMS1_TEMP = 37611;         // I16, °C/10
    constexpr uint16_t BMS1_SOC = 37612;          // U16, %
    constexpr uint16_t BMS1_MAX_TEMP = 37617;     // I16, °C/10
    constexpr uint16_t BMS1_MIN_TEMP = 37618;     // I16, °C/10
    constexpr uint16_t BMS1_MAX_CELL = 37619;     // U16, mV
    constexpr uint16_t BMS1_MIN_CELL = 37620;     // U16, mV
    constexpr uint16_t BMS1_SOH = 37624;          // U16, %
    constexpr uint16_t BMS1_REMAIN_ENERGY = 37632;// U16, Wh/0.1

    // Meter1/CT1 measurements (Table 2-4)
    constexpr uint16_t METER1_CONN = 38801;       // U16, 0=disconnected, 1=connected
    constexpr uint16_t METER1_R_VOLTAGE = 38802;  // I32, V/10
    constexpr uint16_t METER1_S_VOLTAGE = 38804;  // I32, V/10
    constexpr uint16_t METER1_T_VOLTAGE = 38806;  // I32, V/10
    constexpr uint16_t METER1_R_CURRENT = 38808;  // I32, A/1000
    constexpr uint16_t METER1_S_CURRENT = 38810;  // I32, A/1000
    constexpr uint16_t METER1_T_CURRENT = 38812;  // I32, A/1000
    constexpr uint16_t METER1_R_POWER = 38814;    // I32, W/10
    constexpr uint16_t METER1_S_POWER = 38816;    // I32, W/10
    constexpr uint16_t METER1_T_POWER = 38818;    // I32, W/10
    constexpr uint16_t METER1_COMBINED_POWER = 38838; // I32, W/10
    constexpr uint16_t METER1_FREQ = 38844;       // I32, Hz/100

    // Meter2/CT2 measurements
    constexpr uint16_t METER2_CONN = 38901;       // U16
    constexpr uint16_t METER2_R_VOLTAGE = 38902;  // I32, V/10
    constexpr uint16_t METER2_S_VOLTAGE = 38904;  // I32, V/10
    constexpr uint16_t METER2_T_VOLTAGE = 38906;  // I32, V/10
    constexpr uint16_t METER2_R_CURRENT = 38908;  // I32, A/1000
    constexpr uint16_t METER2_S_CURRENT = 38910;  // I32, A/1000
    constexpr uint16_t METER2_T_CURRENT = 38912;  // I32, A/1000
    constexpr uint16_t METER2_COMBINED_POWER = 38938; // I32, W/10
    constexpr uint16_t METER2_FREQ = 38946;       // I32, Hz/100

    // Inverter status & power (Table 2-5)
    constexpr uint16_t PROTOCOL_VERSION = 39000;  // U32
    constexpr uint16_t MODEL_NAME2 = 39002;       // STR, 16
    constexpr uint16_t PN = 39018;                // STR, 16
    constexpr uint16_t SN2 = 39034;               // STR, 16
    constexpr uint16_t MODEL_ID = 39050;          // U16
    constexpr uint16_t NUM_STRINGS = 39051;       // U16
    constexpr uint16_t NUM_MPPT = 39052;          // U16
    constexpr uint16_t RATED_POWER = 39053;       // I32, kW/1000
    constexpr uint16_t MAX_ACTIVE = 39055;        // I32, kW/1000
    constexpr uint16_t MAX_APPARENT = 39057;      // I32, kVA/1000
    constexpr uint16_t MAX_REACTIVE_NEG = 39059;  // I32, kVar/1000
    constexpr uint16_t MAX_REACTIVE_POS = 39061;  // I32, kVar/1000
    constexpr uint16_t STATUS1 = 39063;           // Bitfield16
    constexpr uint16_t STATUS3 = 39065;           // Bitfield32
    constexpr uint16_t ALARM1 = 39066;            // Bitfield16
    constexpr uint16_t ALARM2 = 39067;            // Bitfield16

    // AC power (Table 2-5)
    constexpr uint16_t AC_TOTAL_POWER = 39134;    // I32, kW*1000 (raw in W)
    constexpr uint16_t AC_REACTIVE_POWER = 39136; // I32, kVar*1000

    // PV power (Table 2-5)
    constexpr uint16_t PV_TOTAL_POWER = 39118;    // I32, kW*1000 (raw in W)

    // Energy totals (Table 2-6)
    constexpr uint16_t TOTAL_CHARGE_CAP = 39603;  // U32, kWh/10
    constexpr uint16_t TOTAL_CHARGE_TODAY = 39605;// U32, kWh/10
    constexpr uint16_t TOTAL_DISCHARGE = 39607;   // U32, kWh/10
    constexpr uint16_t TOTAL_DISCHARGE_TODAY = 39609; // U32, kWh/10
    constexpr uint16_t TOTAL_FEEDER = 39611;      // U32, kWh/10
    constexpr uint16_t TOTAL_FEEDER_TODAY = 39613;    // U32, kWh/10
    constexpr uint16_t TOTAL_CONSUMPTION = 39615;   // U32, kWh/10
    constexpr uint16_t TOTAL_CONSUMPTION_TODAY = 39617; // U32, kWh/10
    constexpr uint16_t TOTAL_OUTPUT = 39619;        // U32, kWh/10
    constexpr uint16_t TOTAL_OUTPUT_TODAY = 39621;    // U32, kWh/10
    constexpr uint16_t TOTAL_LOAD = 39623;          // U32, kWh/10
    constexpr uint16_t TOTAL_LOAD_TODAY = 39625;    // U32, kWh/10

} // namespace reg

/**
 * @brief Inverter operational status codes.
 */
enum class InverterStatus : uint16_t {
    STANDBY = 0,
    RESERVED1 = 1,
    OPERATING = 2,
    RESERVED2 = 3,
    RESERVED3 = 4,
    RESERVED4 = 5,
    ERROR = 6,
    RESERVED5 = 7,
};

/**
 * @brief Device information read from the inverter.
 *
 * Contains model info, firmware versions, ratings, and status/alarm bitfields.
 */
struct DeviceInfo {
    std::string model_name;
    std::string serial_number;
    std::string mfg_id;
    uint16_t master_version = 0;
    uint16_t slave_version = 0;
    uint16_t manager_version = 0;
    uint16_t protocol_version = 0;
    uint16_t model_id = 0;
    uint16_t num_strings = 0;
    uint16_t num_mppt = 0;
    float rated_power_kw = 0.0f;
    float max_active_kw = 0.0f;
    float max_apparent_kva = 0.0f;
    float max_reactive_neg_kvar = 0.0f;
    float max_reactive_pos_kvar = 0.0f;
    uint16_t status1 = 0;
    uint32_t status3 = 0;
    uint16_t alarm1 = 0;
    uint16_t alarm2 = 0;
};

/**
 * @brief Meter data for a single CT meter (Meter1/CT1 or Meter2/CT2).
 *
 * Per-phase voltage (V/10), current (A/1000), active power (W/10),
 * and combined three-phase power (W/10) and frequency (Hz/100).
 */
struct MeterData {
    bool connected = false;
    float r_voltage = 0.0f;
    float s_voltage = 0.0f;
    float t_voltage = 0.0f;
    float r_current = 0.0f;
    float s_current = 0.0f;
    float t_current = 0.0f;
    float r_active_power = 0.0f;
    float s_active_power = 0.0f;
    float t_active_power = 0.0f;
    float combined_active_power = 0.0f;
    float frequency = 0.0f;
};

/**
 * @brief Battery Management System (BMS) data.
 *
 * Voltage (V/10), current (A/10), temperature (°C/10), SOC (%),
 * cell voltages (mV), SOH (%), and remaining energy (Wh/0.1).
 */
struct BMSData {
    uint16_t online = 0;
    uint16_t main_control = 0;
    std::string sn;
    uint16_t slave_count = 0;
    float voltage = 0.0f;
    float current = 0.0f;
    float temperature = 0.0f;
    float soc = 0.0f;
    float max_temperature = 0.0f;
    float min_temperature = 0.0f;
    float max_cell_voltage = 0.0f;
    float min_cell_voltage = 0.0f;
    float soh = 0.0f;
    float remain_energy_wh = 0.0f;
};

/**
 * @brief Energy totals read from the inverter.
 *
 * All cumulative energy values are in kWh (raw / 10).
 * Daily energy values are resettable counters.
 */
struct EnergyData {
    float pv_total_power_w = 0.0f;
    float total_charge_kwh = 0.0f;
    float total_charge_today_kwh = 0.0f;
    float total_discharge_kwh = 0.0f;
    float total_discharge_today_kwh = 0.0f;
    float total_feeder_kwh = 0.0f;
    float total_feeder_today_kwh = 0.0f;
    float total_consumption_kwh = 0.0f;
    float total_consumption_today_kwh = 0.0f;
    float total_output_kwh = 0.0f;
    float total_output_today_kwh = 0.0f;
    float total_load_kwh = 0.0f;
    float total_load_today_kwh = 0.0f;
};

/**
 * @brief Complete device snapshot — one fetch from the inverter.
 *
 * Contains all inverter info, meter data, BMS data, and energy totals
 * read in a single transaction batch. The `timestamp` records when
 * the snapshot was taken.
 */
struct DeviceSnapshot {
    DeviceInfo info;
    MeterData meter1;
    MeterData meter2;
    BMSData bms;
    EnergyData energy;
    std::chrono::system_clock::time_point timestamp;
    bool valid = false;
};

/**
 * @brief High-level abstraction for a Solakon ONE inverter.
 *
 * Manages the Modbus TCP connection and provides a single method
 * to fetch a complete snapshot of all inverter data.
 */
class SolakonDevice {
public:
    SolakonDevice() = default;
    ~SolakonDevice() = default;
    SolakonDevice(const SolakonDevice&) = delete;
    SolakonDevice& operator=(const SolakonDevice&) = delete;

    /**
     * @brief Connect to the inverter via Modbus TCP.
     * @param host IP address or hostname.
     * @param port TCP port (default 502).
     * @return true if connection succeeded.
     */
    bool connect(const std::string& host, uint16_t port = 502);

    /**
     * @brief Disconnect from the inverter.
     */
    void disconnect();

    /**
     * @brief Check if connected to the inverter.
     * @return true if connected.
     */
    bool is_connected() const;

    /**
     * @brief Fetch a complete snapshot from the inverter.
     * @return DeviceSnapshot with all inverter data.
     *
     * Reads inverter info, meter data, BMS data, and energy totals
     * in a single batch of Modbus transactions.
     */
    DeviceSnapshot fetch_snapshot();

private:
    class Impl;
    Impl* impl_ = nullptr;
};

} // namespace solakon
