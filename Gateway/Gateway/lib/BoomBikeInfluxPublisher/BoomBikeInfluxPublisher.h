// Handle InfluxDB publishing and WiFi connection for BoomBike Gateway

#ifndef BOOMBIKE_INFLUX_PUBLISHER_H
#define BOOMBIKE_INFLUX_PUBLISHER_H

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <WiFi.h>
#include <Arduino.h>
#include <type_traits>

class BoomBikeInfluxPublisher {
private:
    const char* _ssid;
    const char* _password;
    const char* _influxdb_url;
    const char* _influxdb_org;
    const char* _influxdb_bucket;
    const char* _influxdb_token;
    const char* _timezone;

    InfluxDBClient _influxClient;
    Point _dataPoint;

    void connectToWiFi();
    void validateInfluxConnection();

public:
    BoomBikeInfluxPublisher(const char* ssid, const char* password,
                            const char* influxdb_url, const char* influxdb_org,
                            const char* influxdb_bucket, const char* influxdb_token,
                            const char* timezone = "UTC2");

    void begin();

    template <typename T> void addData(const char* key, T value);

    template <typename T> void addTag(const char* key, T value);

    void publishData(bool clearAfterPublish = true);
};

#endif // BOOMBIKE_INFLUX_PUBLISHER_H

// Templates for adding data and tags
template <typename T>
void BoomBikeInfluxPublisher::addData(const char* key, T value) {
    if (std::is_floating_point<T>::value) {
        String valStr = String(value, 5); // 5 decimal places
        _dataPoint.addField(key, valStr);
    } else if (std::is_same<T, long>::value) {
        _dataPoint.addField(key, String(value));
    } else {
        _dataPoint.addField(key, value);
    }
}

template <typename T>
void BoomBikeInfluxPublisher::addTag(const char* key, T value) {
    if (std::is_floating_point<T>::value) {
        String valStr = String(value, 5); // 5 decimal places
        _dataPoint.addTag(key, valStr);
    } else if (std::is_same<T, long>::value) {
        _dataPoint.addTag(key, String(value));
    } else {
        _dataPoint.addTag(key, value);
    }
}
