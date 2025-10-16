import asyncio
import json
import os
from gmqtt import Client as MQTTClient
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS, WriteOptions

# MQTT configuration
BROKER_HOST = "mosquitto"
BROKER_PORT = 1883
TOPIC = "bike/data"

# InfluxDB configuration
INFLUX_URL = "http://influxdb:8086"
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
INFLUX_ORG = "myorg"
INFLUX_BUCKET = "bike_data"

class MQTTInfluxBridge:

    def __init__(self):
        # InfluxDB client
        self.influx = InfluxDBClient(
            url=INFLUX_URL,
            token=INFLUX_TOKEN,
            org=INFLUX_ORG
        )

        self.write_api = self.influx.write_api(write_options=WriteOptions(write_type=SYNCHRONOUS))

        # MQTT client
        self.mqtt_client = MQTTClient("mqtt_bridge_client")
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message
        self.mqtt_client.on_disconnect = self.on_disconnect
        self.mqtt_client.on_subscribe = self.on_subscribe

    async def connect_mqtt(self):
        await self.mqtt_client.connect(BROKER_HOST, BROKER_PORT)
        self.mqtt_client.subscribe(TOPIC)

    def on_connect(self, client, flags, rc, properties):
        print(f"[MQTT] Connected with result code {rc}")

    def on_disconnect(self, client, packet, exc=None):
        print("[MQTT] Disconnected from broker")

    def on_subscribe(self, client, mid, qos, properties):
        print(f"[MQTT] Subscribed to topic {TOPIC}")

    def on_message(self, client, topic, payload, qos, properties):
        try:
            data = json.loads(payload.decode())
        except json.JSONDecodeError:
            print(f"[MQTT] Invalid JSON received: {payload.decode()}")
            return

        print(f"[MQTT] Received data: {data}")

        # Create new InfluxDB Point
        point = Point("bike").tag("source", "mqtt")

        # Dynamically add all fields from the JSON data
        for key, value in data.items():
            try:
                # Convert to float if possible, otherwise store as string
                num_value = float(value)
                point.field(key, num_value)
            except (ValueError, TypeError):
                point.field(key, str(value))
                print(f"[MQTT] Invalid value for {key}: {value}")
                
        # Write to InfluxDB
        try:
            self.write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=point)
            print("[InfluxDB] Data written successfully")
        except Exception as e:
            print(f"[InfluxDB] Error writing data: {e}")

    async def start(self):
        await self.connect_mqtt()
        # Keep the bridge running
        await asyncio.Event().wait()


async def main():
    bridge = MQTTInfluxBridge()
    await bridge.start()


if __name__ == "__main__":
    asyncio.run(main())
