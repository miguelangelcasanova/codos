// WiFi configuración
const char* ssid = "??????";
const char* password = "???????";

// MQTT configuración
const char* mqtt_server = "192.168.1.???";
const int mqtt_port = 1883;
const char* mqtt_id = "airqualitysensor";
const char* mqtt_sub_topic_healthcheck = "/home/airqualitysensor";
const char* mqtt_pub_topic_co2 = "/home/airqualitysensor/co2";
const char* mqtt_pub_topic_tvoc = "/home/airqualitysensor/tvoc";
const char* mqtt_sub_topic_operation = "/home/airqualitysensor/operation";

// Otros parámetros
const int update_time_sensors = 5000;
const int PIN_STRIP_1 = 0;
const int NUMPIXELS_STRIP_1 = 3;
const int CO2_MIN = 400;
const int CO2_MAX = 1500;
