#include "BoomBikeInfluxPublisher.h"

BoomBikeInfluxPublisher::BoomBikeInfluxPublisher(const char* ssid, const char* password,
                                               const char* influxdb_url, const char* influxdb_org,
                                               const char* influxdb_bucket, const char* influxdb_token,
                                               const char* timezone)
    : _ssid(ssid), _password(password),
      _influxdb_url(influxdb_url), _influxdb_org(influxdb_org),
      _influxdb_bucket(influxdb_bucket), _influxdb_token(influxdb_token),
      _timezone(timezone),
      _influxClient(influxdb_url, influxdb_org, influxdb_bucket, influxdb_token, InfluxDbCloud2CACert),
      _dataPoint("boombike_data") {
        
}

void BoomBikeInfluxPublisher::connectToWiFi() {
    Serial.print("Connecting to WiFi SSID: ");
    Serial.println(_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    timeSync(_timezone, "pool.ntp.org", "time.nist.gov");
    Serial.println("Time synchronized");
}

void BoomBikeInfluxPublisher::validateInfluxConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Attempting to reconnect...");
        connectToWiFi();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Failed to reconnect to WiFi. Cannot validate InfluxDB connection.");
            return;
        }
    }
    if (_influxClient.validateConnection()) {
        Serial.println("Connected to InfluxDB: " + _influxClient.getServerUrl());
    } else {
        Serial.print("InfluxDB connection failed: " + _influxClient.getLastErrorMessage());
    }
}

void BoomBikeInfluxPublisher::begin() {
    connectToWiFi();
    validateInfluxConnection();
    // Data point to verify functionality
    addData("status", "connected");
    publishData();
}

void BoomBikeInfluxPublisher::publishData(bool clearAfterPublish) {
    if (!_dataPoint.hasFields()) {
        Serial.println("No data to publish");
        return;
    }
    if (_influxClient.writePoint(_dataPoint)) {
        validateInfluxConnection();
        Serial.println("Data published to InfluxDB");
        if (clearAfterPublish) {
            _dataPoint.clearFields();
        }
    } else {
        Serial.print("Failed to write data: " + _influxClient.getLastErrorMessage());
    }
}
