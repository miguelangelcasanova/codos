#ifndef InfluxDB_h

// base de datos y credenciales
const char INFLUX_IP[] = "123.123.123.123"; // IP del servidor influxDB
const char INFLUX_USER[] = "username"; // username (if authorization is enabled)
const char INFLUX_PASS[] = "password"; // password (if authorization is enabled)
const char INFLUX_DATABASE[] = "CCS811EVAL"; // nombre de la base de datos, propongo CCS811EVAL 
const char INFLUX_MEASUREMENT[] = "TELEGRAM-NICK"; // tag para identificar de quien son los datos, propongo el Telegram-Nick - ejemplo "@Andreas_IBZ"

// certificado(s) del servidor
const char ROOT_CERT[] = "-----BEGIN CERTIFICATE-----\n\
.....\n\
.....\n\
-----END CERTIFICATE-----\n";

#endif
