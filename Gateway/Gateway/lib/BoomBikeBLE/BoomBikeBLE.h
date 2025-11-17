// BoomBikeBLE.h
#ifndef BOOMBIKEBLE_H
#define BOOMBIKEBLE_H

#include <NimBLEDevice.h>
#include <functional>

// Callback function type for when data is received from a node
using DataReceivedCallback = std::function<void(const std::string& address, const std::string& data)>;

class BoomBikeBLE : public NimBLEScanCallbacks {
public:
    BoomBikeBLE(const char* serviceUUID, const char* characteristicUUID);
    ~BoomBikeBLE();
    void begin();
    void loop(); // Should be called in the main loop to process BLE events
    void onDataReceived(DataReceivedCallback callback);
    void setScanParameters(uint32_t intervalMs, uint32_t durationMs);

private:
    // From NimBLEScanCallbacks
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override;

    bool connectAndRead(const NimBLEAdvertisedDevice* device);

    NimBLEUUID m_serviceUUID;
    NimBLEUUID m_characteristicUUID;
    DataReceivedCallback m_dataCallback = nullptr;

    // State for connecting outside of the callback
    bool m_doConnect = false;
    NimBLEAdvertisedDevice* m_pAdvertisedDevice = nullptr;
    
    // Constants for scanning
    static const uint32_t SCAN_INTERVAL = 2000; // Time between the start of each scan (ms)
    static const uint32_t SCAN_DURATION = 600;  // Duration of each scan (ms)
    uint32_t m_lastScanTime = 0;
};

#endif // BOOMBIKEBLE_H
