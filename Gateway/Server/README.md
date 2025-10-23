# Local Setup using Docker Containers

As mentioned in the introduction in the parent folder, it is possible to host both the InfluxDB database and the Grafana dashboard locally using Docker containers. This section provides instructions on how to set up and run these containers on a personal server.

## System overview

```
Bike --BLE--> Gateway
                 | (MQTT publish over WiFi)
                 v
          Mosquitto Broker
                 | (MQTT subscribe)
                 v
         MQTT Bridge (Python)
                 | (HTTP/Influx Line Protocol)
                 v
              InfluxDB 
                 | (HTTP API)
                 v
              Grafana 
```

## Workflow

1. The smart bike sends data over BLE long range to the gateway (ESP32c3).
2. The gateway publishes the received data to an MQTT broker over a WiFi connection. The MQTT broker is hosted in a Docker container on the personal server.
3. A Python script running in another Docker container subscribes to the MQTT broker and receives the published data.
4. The Python script then writes the received data to an InfluxDB database using the Influx Line Protocol over HTTP. The InfluxDB database is also hosted in a Docker container on the personal server.
5. Finally, Grafana, running in its own Docker container, accesses the InfluxDB database using its HTTP API to visualize the data on a dashboard.

## Deployment

Simply start the docker containers. Navigate to http://SERVER-IP:3000 for the dashboard. The credentials can be changed inside the .env file before starting the docker containers.

default username: admin\
default password: admin123

## Test MQTT commands

on a Windows machine:
```
mosquitto_pub -h localhost -t bike/data -m "{\"speed\":15,\"temp\":20.1}"
```

Or just use the gateway to send MQTT commands. Make sure to publish on a topic that the MQTT bridge is subscribed to!

## Change default dashboard
Go to the Grafana server and set up the dashboard as you want it to be. Then go to edit->settings->JSON Model and copy the json configuration. Put this in /grafana/dashboards/BoomBike.json and it will be the default configuration when deploying the container.
