/*************************************************************
  แก้ไขให้รับด้วย Rx แท้เท่านั้น
 *************************************************************/
#define BLYNK_TEMPLATE_ID "TMPL2RyOZlTf"
#define BLYNK_DEVICE_NAME "ESP8266 DTH"
#define BLYNK_AUTH_TOKEN "lA37jFm1oDbG7z0fmBHZJxV_ruBpLObW";

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <SoftwareSerial.h>
SoftwareSerial NodeSerial(14, 12); // ขา Rx(D5), Tx(D6)

char auth[] = BLYNK_AUTH_TOKEN;

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
WiFiManager wifiManager;
String locIP;

int IntRst = 15; // Pin D8=15,

#define DHTPIN 5 // D1
#define DHTTYPE DHT22

#define LED 16 // LED BUILD IN
#define TRIGGER_PIN 0     // Button     Reset Network     ( Pin D3 )
#define LED_PIN 2         // LED        connection status ( Pin D4 )

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
String chkData = "";
String combData = "";
void sendSensor()
{
  digitalWrite(LED, LOW);

  int lev = analogRead(A0);
  Serial.println(" " + String(lev));

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed");
    return;
  }
  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);
  combData = String(h)+String(t)+String(lev);
  if (chkData != combData){
    chkData = combData;
    NodeSerial.println("|" + String(h) + "," + String(t) + "," + String(lev) + "|");
    Serial.print(h);
    Serial.print(" ");
    Serial.print(t);
    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
    digitalWrite(LED, HIGH);  
  }
}

void setup()
{
  // Debug console
  Serial.begin(9600);
  NodeSerial.begin(9600);
  pinMode(LED, OUTPUT);

  pinMode(TRIGGER_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("VRU ME.BoxPlanting WiFi")) {
    ESP.reset();
    delay(1000);
  }
  Blynk.config(auth);

  dht.begin();
  timer.setInterval(1500L, sendSensor);

}

void loop()
{

  Blynk.run();
  timer.run();
  int count = 0;

  // Reset the network
//  Serial.print(digitalRead(TRIGGER_PIN));
//  delay(1000);
  while (digitalRead(TRIGGER_PIN) == LOW) {
    delay(100);
    count++;
    Serial.println(count);
    if (count > 30) {
      //ESP.eraseConfig();
      WiFi.disconnect();
      Serial.println("|Reset Wifi|");
      NodeSerial.print("|Reset Wifi|");
      wifiManager.resetSettings();
      digitalWrite(LED_PIN, LOW);
      delay(1000);
      count = 0;
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      ESP.reset();
      delay(1000);
    }
  }
}
