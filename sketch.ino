#include <WiFi.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include <ArduinoJson.h>  

#define PIN_TRIG 26
#define PIN_ECHO 25
#define LOWLED   18
#define MIDLED   19
#define HIGHLED  21
#define MOTOR    27

#define WIFI_AP "Wokwi-GUEST"
#define WIFI_PASS ""

#define TB_SERVER "thingsboard.cloud"
#define TOKEN "****"

constexpr uint16_t MAX_MESSAGE_SIZE = 128U;
WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

unsigned int level = 0;

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.begin(WIFI_AP, WIFI_PASS, 6);
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi.");
  } else {
    Serial.println("\nConnected to WiFi");
  }
}

void connectToThingsBoard() {
  if (!tb.connected()) {
    Serial.println("Connecting to ThingsBoard server");
    
    if (!tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Failed to connect to ThingsBoard");
    } else {
      Serial.println("Connected to ThingsBoard");
    }
  }
}

void sendDataToThingsBoard(unsigned int levelCm) {
  // 1) Cria um JsonDocument com capacidade suficiente
  StaticJsonDocument<64> doc;
  doc["water_level_cm"] = levelCm;

  // 2) Serializa para um buffer para medir o tamanho exato
  char jsonBuf[64];
  size_t jsonSize = serializeJson(doc, jsonBuf, sizeof(jsonBuf));

  // 3) Envia ao ThingsBoard usando o overload correto
  bool ok = tb.sendTelemetryJson(doc, jsonSize);
  if (!ok) {
    Serial.println("Failed to send telemetry");
  } else {
    Serial.print("Data sent: ");
    Serial.println(jsonBuf);
  }
}

void setup() {
  pinMode(LOWLED,OUTPUT);
  pinMode(MIDLED,OUTPUT);
  pinMode(HIGHLED,OUTPUT);
  pinMode(MOTOR,OUTPUT);
  digitalWrite(LOWLED,HIGH);
  digitalWrite(MIDLED,HIGH);
  digitalWrite(HIGHLED,HIGH);
  digitalWrite(MOTOR,LOW);

  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  connectToWiFi();
  connectToThingsBoard();
}

void loop() {
  connectToWiFi();
  // Start a new measurement:
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Read the result:
  int duration = pulseIn(PIN_ECHO, HIGH);
  Serial.print("Distance in CM: ");
  Serial.println(duration / 58);
  Serial.print("Distance in inches: ");
  Serial.println(duration / 148);
  
  level = duration / 58;

  if (level < 10)
  {
        digitalWrite(LOWLED,LOW);
        digitalWrite(MOTOR,HIGH);
        digitalWrite(HIGHLED,HIGH);
        digitalWrite(MIDLED,HIGH);
  }
  else if ((level > 20) && (level <40))
  {
        digitalWrite(LOWLED,HIGH);
        digitalWrite(HIGHLED,HIGH);
        digitalWrite(MIDLED,LOW);
  }
  else if (level >= 80 )
  {
        digitalWrite(HIGHLED,LOW);
        digitalWrite(MIDLED,HIGH);
        digitalWrite(LOWLED,HIGH);
        digitalWrite(MOTOR,LOW);
  }

    if (!tb.connected()) {
    connectToThingsBoard();
  }

  sendDataToThingsBoard(level);
  delay(10);

  tb.loop();
}
