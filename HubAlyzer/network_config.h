#pragma once

#ifdef ENABLE_OTA
    constexpr const char* wifiSsid = "YOUR_WIFI_SSID";
    constexpr const char* wifiPassword = "YOUR_WIFI_PASSWORD";
#endif

#ifdef ENABLE_BLUETOOTH
    const char *bluetoothPinCode = "YOUR_BT_PINCODE";
#endif
