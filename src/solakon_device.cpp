#include "solakon_device.h"
#include "modbus_client.h"

#include <span>
#include <vector>
#include <cstring>

namespace solakon {

// Implementation class (pimpl)
class SolakonDevice::Impl {
public:
    modbus::Client client_;

    bool connect(const std::string& host, uint16_t port = 502) {
        auto ec = client_.connect(host, port);
        return !ec;
    }

    void disconnect() {
        client_.disconnect();
    }

    bool is_connected() const {
        return client_.is_connected();
    }

    DeviceSnapshot fetch_snapshot() {
        DeviceSnapshot snap{};
        snap.timestamp = std::chrono::system_clock::now();

        // Read all data in batches
        read_inverter_info(snap.info);
        read_meter_data(snap.meter1, snap.meter2);
        read_battery_data(snap.bms);
        read_energy_data(snap.energy);

        snap.valid = true;
        return snap;
    }

private:
    void read_inverter_info(DeviceInfo& info) {
        // Protocol version (U32 at 39000)
        uint32_t val;
        if (!client_.read_u32(reg::PROTOCOL_VERSION, val)) {
            info.protocol_version = val;
        }

        // Model name (STR at 39002, 16 regs)
        client_.read_string(reg::MODEL_NAME2, 16, info.model_name);

        // PN (STR at 39018, 16 regs)
        client_.read_string(reg::PN, 16, info.serial_number);

        // Model ID, num strings, num MPPT
        client_.read_u16(reg::MODEL_ID, info.model_id);
        client_.read_u16(reg::NUM_STRINGS, info.num_strings);
        client_.read_u16(reg::NUM_MPPT, info.num_mppt);

        // Rated power (I32 at 39053, kW/1000)
        int32_t ival = 0;
        if (!client_.read_i32(reg::RATED_POWER, ival)) {
            info.rated_power_kw = ival / 1000.0f;
        }

        // Max active power (I32 at 39055, kW/1000)
        if (!client_.read_i32(reg::MAX_ACTIVE, ival)) {
            info.max_active_kw = ival / 1000.0f;
        }

        // Max apparent (I32 at 39057, kVA/1000)
        if (!client_.read_i32(reg::MAX_APPARENT, ival)) {
            info.max_apparent_kva = ival / 1000.0f;
        }

        // Max reactive (I32 at 39059, kVar/1000)
        if (!client_.read_i32(reg::MAX_REACTIVE_NEG, ival)) {
            info.max_reactive_neg_kvar = ival / 1000.0f;
        }

        // Status registers
        client_.read_u16(reg::STATUS1, info.status1);
        uint32_t s3;
        client_.read_u32(reg::STATUS3, s3);
        info.status3 = s3;
        client_.read_u16(reg::ALARM1, info.alarm1);
        client_.read_u16(reg::ALARM2, info.alarm2);
    }

    void read_meter_data(MeterData& m1, MeterData& m2) {
        // Meter1/CT1 (Table 2-4)
        uint16_t uval = 0;
        int32_t ival = 0;

        client_.read_u16(reg::METER1_CONN, uval);
        m1.connected = (uval == 1);

        client_.read_i32(reg::METER1_R_VOLTAGE, ival);
        m1.r_voltage = ival / 10.0f;

        client_.read_i32(reg::METER1_S_VOLTAGE, ival);
        m1.s_voltage = ival / 10.0f;

        client_.read_i32(reg::METER1_T_VOLTAGE, ival);
        m1.t_voltage = ival / 10.0f;

        client_.read_i32(reg::METER1_R_CURRENT, ival);
        m1.r_current = ival / 1000.0f;

        client_.read_i32(reg::METER1_S_CURRENT, ival);
        m1.s_current = ival / 1000.0f;

        client_.read_i32(reg::METER1_T_CURRENT, ival);
        m1.t_current = ival / 1000.0f;

        client_.read_i32(reg::METER1_R_POWER, ival);
        m1.r_active_power = ival / 10.0f;

        client_.read_i32(reg::METER1_S_POWER, ival);
        m1.s_active_power = ival / 10.0f;

        client_.read_i32(reg::METER1_T_POWER, ival);
        m1.t_active_power = ival / 10.0f;

        client_.read_i32(reg::METER1_COMBINED_POWER, ival);
        m1.combined_active_power = ival / 10.0f;

        client_.read_i32(reg::METER1_FREQ, ival);
        m1.frequency = ival / 100.0f;

        // Meter2/CT2
        client_.read_u16(reg::METER2_CONN, uval);
        m2.connected = (uval == 1);

        client_.read_i32(reg::METER2_R_VOLTAGE, ival);
        m2.r_voltage = ival / 10.0f;

        client_.read_i32(reg::METER2_S_VOLTAGE, ival);
        m2.s_voltage = ival / 10.0f;

        client_.read_i32(reg::METER2_T_VOLTAGE, ival);
        m2.t_voltage = ival / 10.0f;

        client_.read_i32(reg::METER2_R_CURRENT, ival);
        m2.r_current = ival / 1000.0f;

        client_.read_i32(reg::METER2_S_CURRENT, ival);
        m2.s_current = ival / 1000.0f;

        client_.read_i32(reg::METER2_T_CURRENT, ival);
        m2.t_current = ival / 1000.0f;

        client_.read_i32(reg::METER2_COMBINED_POWER, ival);
        m2.combined_active_power = ival / 10.0f;

        client_.read_i32(reg::METER2_FREQ, ival);
        m2.frequency = ival / 100.0f;
    }

    void read_battery_data(BMSData& bms) {
        uint16_t uval = 0;
        int16_t ival = 0;

        client_.read_u16(reg::BMS1, uval);
        bms.online = uval;

        client_.read_u16(reg::BMS1_MAIN, uval);
        bms.main_control = uval;

        client_.read_string(reg::BMS1_SN, 16, bms.sn);

        client_.read_u16(reg::BMS1_SLAVE_COUNT, uval);
        bms.slave_count = uval;

        client_.read_u16(reg::BMS1_VOLTAGE, uval);
        bms.voltage = uval / 10.0f;

        client_.read_i16(reg::BMS1_CURRENT, ival);
        bms.current = ival / 10.0f;

        client_.read_i16(reg::BMS1_TEMP, ival);
        bms.temperature = ival / 10.0f;

        client_.read_u16(reg::BMS1_SOC, uval);
        bms.soc = uval;

        client_.read_i16(reg::BMS1_MAX_TEMP, ival);
        bms.max_temperature = ival / 10.0f;

        client_.read_i16(reg::BMS1_MIN_TEMP, ival);
        bms.min_temperature = ival / 10.0f;

        client_.read_u16(reg::BMS1_MAX_CELL, uval);
        bms.max_cell_voltage = uval;

        client_.read_u16(reg::BMS1_MIN_CELL, uval);
        bms.min_cell_voltage = uval;

        client_.read_u16(reg::BMS1_SOH, uval);
        bms.soh = uval;

        client_.read_u16(reg::BMS1_REMAIN_ENERGY, uval);
        bms.remain_energy_wh = uval / 0.1f;
    }

    void read_energy_data(EnergyData& energy) {
        uint32_t uval = 0;

        // PV total power (39118, I32, kW*1000) -> raw is already in Watts
        int32_t pv_raw = 0;
        if (!client_.read_i32(reg::PV_TOTAL_POWER, pv_raw)) {
            energy.pv_total_power_w = static_cast<float>(pv_raw);
        }

        // Total charge capacity (U32 at 39603, kWh/10)
        if (!client_.read_u32(reg::TOTAL_CHARGE_CAP, uval)) {
            energy.total_charge_kwh = uval / 10.0f;
        }

        // Total charge today (U32 at 39605, kWh/10)
        if (!client_.read_u32(reg::TOTAL_CHARGE_TODAY, uval)) {
            energy.total_charge_today_kwh = uval / 10.0f;
        }

        // Total discharge (U32 at 39607, kWh/10)
        if (!client_.read_u32(reg::TOTAL_DISCHARGE, uval)) {
            energy.total_discharge_kwh = uval / 10.0f;
        }

        // Total discharge today (U32 at 39609, kWh/10)
        if (!client_.read_u32(reg::TOTAL_DISCHARGE_TODAY, uval)) {
            energy.total_discharge_today_kwh = uval / 10.0f;
        }

        // Total feeder (U32 at 39611, kWh/10)
        if (!client_.read_u32(reg::TOTAL_FEEDER, uval)) {
            energy.total_feeder_kwh = uval / 10.0f;
        }

        // Total feeder today (U32 at 39613, kWh/10)
        if (!client_.read_u32(reg::TOTAL_FEEDER_TODAY, uval)) {
            energy.total_feeder_today_kwh = uval / 10.0f;
        }

        // Total consumption (U32 at 39615, kWh/10)
        if (!client_.read_u32(reg::TOTAL_CONSUMPTION, uval)) {
            energy.total_consumption_kwh = uval / 10.0f;
        }

        // Total consumption today (U32 at 39617, kWh/10)
        if (!client_.read_u32(reg::TOTAL_CONSUMPTION_TODAY, uval)) {
            energy.total_consumption_today_kwh = uval / 10.0f;
        }

        // Total output (U32 at 39619, kWh/10)
        if (!client_.read_u32(reg::TOTAL_OUTPUT, uval)) {
            energy.total_output_kwh = uval / 10.0f;
        }

        // Total output today (U32 at 39621, kWh/10)
        if (!client_.read_u32(reg::TOTAL_OUTPUT_TODAY, uval)) {
            energy.total_output_today_kwh = uval / 10.0f;
        }

        // Total load (U32 at 39623, kWh/10)
        if (!client_.read_u32(reg::TOTAL_LOAD, uval)) {
            energy.total_load_kwh = uval / 10.0f;
        }

        // Total load today (U32 at 39625, kWh/10)
        if (!client_.read_u32(reg::TOTAL_LOAD_TODAY, uval)) {
            energy.total_load_today_kwh = uval / 10.0f;
        }
    }
};

// SolakonDevice implementation
bool SolakonDevice::connect(const std::string& host, uint16_t port) {
    if (!impl_) {
        impl_ = new Impl();
    }
    return impl_->connect(host, port);
}

void SolakonDevice::disconnect() {
    if (impl_) {
        impl_->disconnect();
    }
}

bool SolakonDevice::is_connected() const {
    return impl_ && impl_->is_connected();
}

DeviceSnapshot SolakonDevice::fetch_snapshot() {
    if (!impl_) {
        return DeviceSnapshot{};
    }
    return impl_->fetch_snapshot();
}

} // namespace solakon
