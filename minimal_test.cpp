#include "solakon_device.h"
#include <cstdio>
#include <chrono>

int main() {
    solakon::SolakonDevice device;
    
    std::printf("Connecting...\n");
    if (!device.connect("192.168.178.121")) {
        std::printf("FAIL: connect\n");
        return 1;
    }
    std::printf("Connected!\n");
    
    std::printf("Fetching snapshot...\n");
    auto snap = device.fetch_snapshot();
    std::printf("Snapshot done! valid=%d\n", snap.valid);
    
    std::printf("status1=%u status3=%u alarm1=%u alarm2=%u\n",
        snap.info.status1, snap.info.status3, snap.info.alarm1, snap.info.alarm2);
    std::printf("rated_power=%f kW\n", snap.info.rated_power_kw);
    std::printf("max_active=%f kW\n", snap.info.max_active_kw);
    std::printf("model='%s' pn='%s'\n", snap.info.model_name.c_str(), snap.info.serial_number.c_str());
    std::printf("meter1.conn=%d r_v=%f s_v=%f t_v=%f\n",
        snap.meter1.connected, snap.meter1.r_voltage, snap.meter1.s_voltage, snap.meter1.t_voltage);
    std::printf("meter1.r_cur=%f s_cur=%f t_cur=%f\n",
        snap.meter1.r_current, snap.meter1.s_current, snap.meter1.t_current);
    std::printf("meter1.r_pwr=%f s_pwr=%f t_pwr=%f combined=%f freq=%f\n",
        snap.meter1.r_active_power, snap.meter1.s_active_power, snap.meter1.t_active_power,
        snap.meter1.combined_active_power, snap.meter1.frequency);
    std::printf("meter2.conn=%d r_v=%f combined=%f freq=%f\n",
        snap.meter2.connected, snap.meter2.r_voltage, snap.meter2.combined_active_power, snap.meter2.frequency);
    std::printf("bms.online=%u soc=%f volt=%f cur=%f temp=%f\n",
        snap.bms.online, snap.bms.soc, snap.bms.voltage, snap.bms.current, snap.bms.temperature);
    std::printf("energy.pv_power=%f charge=%f charge_today=%f\n",
        snap.energy.pv_total_power_w, snap.energy.total_charge_kwh, snap.energy.total_charge_today_kwh);
    std::printf("energy.discharge=%f discharge_today=%f feeder=%f\n",
        snap.energy.total_discharge_kwh, snap.energy.total_discharge_today_kwh, snap.energy.total_feeder_kwh);
    std::printf("energy.consumption=%f consumption_today=%f output=%f output_today=%f\n",
        snap.energy.total_consumption_kwh, snap.energy.total_consumption_today_kwh,
        snap.energy.total_output_kwh, snap.energy.total_output_today_kwh);
    std::printf("energy.load=%f load_today=%f\n",
        snap.energy.total_load_kwh, snap.energy.total_load_today_kwh);
    
    device.disconnect();
    std::printf("Done.\n");
    return 0;
}
