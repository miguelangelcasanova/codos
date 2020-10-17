# Influx-Arduino
Simple library for writing to InfluxDB from an Arduino device

For more details on how this came about, see this [Medium post](https://medium.com/@teebr/iot-with-an-esp32-influxdb-and-grafana-54abc9575fb2).

This has been tested with an ESP32: any board should work as long as it has an HTTPClient library and an encryption library (e.g mbed TLS) to go with it.


There is now [another branch](https://github.com/teebr/Influx-Arduino/tree/m0-wifi) which should work when an ATCWINC1500 WiFi module is used (see the branch readme for more info). I'm currently working out how to merge the two as the WiFiClient classes are quite different.
