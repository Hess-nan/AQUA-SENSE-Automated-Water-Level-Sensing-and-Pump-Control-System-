#ifndef SECRET_H
#define SECRET_H
#include <Arduino.h>

constexpr char WIFI_SSID[] = "Nama_Wifi";
constexpr char WIFI_PASSWORD[] = "Password_Wifi";

constexpr char MQTT_BROKER[] = "broker.hivemq.com";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_TOPIC[] = "toren/level";

#endif