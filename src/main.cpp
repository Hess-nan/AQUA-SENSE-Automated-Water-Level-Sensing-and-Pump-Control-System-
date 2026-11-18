#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "secret.h"

// Definisi pin JSN-SR04T 
#define pintrig 12
#define pinecho 13

// Definisi pin relay pompa
#define pinRelay 32

// setpoint 
#define setpoint  30 // cm

// Threshold error untuk menghidupkan/mematikan pompa
#define ERROR_THRESHOLD  5.0 // cm

bool statusPompa = false;
bool statusToren = false;

// Parameter Toren (HARUS DITENTUKAN DULU)
#define diameter 69 //cm
#define tinggiToren 97 //cm
#define tinggiSensor 67 //cm
#define jarakMinSensor 20 //cm



// Konfigurasi MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// topic MQTT
const char* topic = "toren/level";

// membuat objek WiFiClient dan PubSubClient
WiFiClient espClient;
PubSubClient client(espClient);

// Koneksi Wifi
void setup_wifi(){
  delay(10);
  Serial.println("Menghubungkan ke WiFi....");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi terhubung");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

// Konekso MQTT
void reconnect(){
  while(!client.connected()){
    Serial.print("Menghubungkan ke MQTT");
    String clientID = "ESP32Client-";
    clientID += String(random(0xffff), HEX);

    if (client.connect(clientID.c_str())){
      Serial.println("Terhubung ke broker MQTT");
    }else{
      Serial.print("Gagal, state: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// Fungsi mengirim Json 
void sendJSON(float jarakRatarata, bool statusPompa, float volumeTerukur, bool statusToren, float persentase ){
  JsonDocument doc;

  doc["jarak"] = jarakRatarata;
  doc["statusPompa"] = statusPompa;
  doc["volume"] = volumeTerukur;
  doc["statusVolume"] = statusToren;
  doc["persentase"] = persentase;
  
  // mengubah JSON menjadi string 
  String jsonData;
  serializeJson(doc, jsonData);
  
  // menampilkan data Json ke serial monitor
  Serial.println(jsonData);

  // mengirim data json ke broker
  if (client.publish(MQTT_TOPIC, jsonData.c_str())){
    Serial.println("Data berhasil dikirim ke broker MQTT");
  } else{
    Serial.println("Gagal mengirim data");
  }
}

// Millis untuk pengirima data ke broker MQTT
unsigned long intervalMQTT = 500;
unsigned long lastMQTT = 0;

// Millis untuk pembacaan JSN-SR04T
unsigned long intervalJSN = 60;
unsigned long lastJSN = 0;

void setup() {
  Serial.begin(9600);
  pinMode(pintrig, OUTPUT);
  pinMode(pinecho, INPUT);
  pinMode(pinRelay, OUTPUT);

  setup_wifi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
}

void loop() {

  if (WiFi.status() != WL_CONNECTED){
    setup_wifi();
  }

  if (!client.connected()){
    reconnect();
  }

  client.loop();
  
  unsigned long currentMillis = millis();

  float totalJarak = 0;
  int sampel = 0;
  float jarakRatarata = 0;


  if (currentMillis - lastJSN >= intervalJSN){
    lastJSN = currentMillis;
    digitalWrite(pintrig, LOW);
    delayMicroseconds(2);
    digitalWrite(pintrig, HIGH);
    delayMicroseconds(20);
    digitalWrite(pintrig, LOW);
    
    long durasi = pulseIn(pinecho, HIGH);
    long jarak = durasi * 0.034 / 2; // cm

    if (jarak > 20 && jarak < 400){
      totalJarak += jarak;
      sampel ++;
    }
}

  jarakRatarata = totalJarak / sampel;
  float volumeTotal = (PI * pow(diameter, 2)/4) * (tinggiToren - tinggiSensor - jarakMinSensor);
  float volumeTerukur = (PI * pow(diameter, 2)/4) * jarakRatarata; //cm^3
  float persentase = (volumeTerukur / volumeTotal) * 100; // %
  float error = setpoint - jarakRatarata;
  
  // kontrol pompa
  if (error > ERROR_THRESHOLD && !statusPompa){
    digitalWrite(pinRelay, HIGH);
    statusPompa = true; 
    statusToren = false;

  }else if (error < ERROR_THRESHOLD && statusPompa){
    digitalWrite(pinRelay, LOW);
    statusPompa = false;
    statusToren = true;
  }



  // mengirim data Json
  if (currentMillis - lastMQTT >= intervalMQTT){
    lastMQTT = currentMillis;
    sendJSON(jarakRatarata, statusPompa, volumeTerukur, statusToren, persentase);


    // Tampilan Variabel 
    Serial.print("Jarak: ");Serial.print(jarakRatarata);
    Serial.print(" | Status Pompa: ");Serial.print(statusPompa ? "ON" : "OFF");
    Serial.print(" | Volume: ");Serial.print(volumeTerukur);
    Serial.print(" | Status Volume: ");Serial.print( statusToren ? "Penuh" : "Kosong");
    Serial.print(" | Persentase: ");Serial.println(persentase);
    
  }
  
}

/**
 data yang mau dikirim ke broker MQTT dalam format JSON
  {
    "jarak": jarakRatarata,
    "statusPompa": statusPompa
    "Volume": volume
    "statusVolume": statusVolume
    "persentase": persentase
  }
 */





