# BoomBike

This project contains a simple setup for monitoring data from a smart bike. The bike sends data over BLE long range to a gateway (ESP32c3 in this case) which then publishes the data to an MQTT broker over WiFi. On the server side, a python script subscribes to the MQTT broker and stores the data in an InfluxDB database. Finally, Grafana is used to visualize the data.
The whole server setup is containerized using Docker for easy deployment and scaling.

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

## Structure

- /gateway: Code for the ESP32c3 gateway to receive BLE data and publish to MQTT broker.
- /Server: The Docker containers are set up in the docker-compose.yml file.

# Server setup

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
