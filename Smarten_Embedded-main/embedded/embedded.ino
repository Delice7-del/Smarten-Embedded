#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ====== Wi-Fi SETTINGS ======
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ====== MQTT SETTINGS ======
const char* mqtt_server = "BROKER_IP_OR_URL"; // e.g., "192.168.1.10" or "broker.hivemq.com"
const int mqtt_port = 1883;
const char* flow_topic = "pipe/flow";
const char* leak_topic = "pipe/leak";

WiFiClient espClient;
PubSubClient client(espClient);

// ====== SENSOR PINS ======
const int flowPin = D1;   // Flow sensor output
const int leakPin = D2;   // Leak sensor analog/digital output

// ====== FLOW SENSOR VARIABLES ======
volatile int pulseCount = 0;  // Counts pulses from the flow sensor
float flowRate = 0.0;         // Liters per minute

// ====== ISR for Flow Sensor ======
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// ====== CONNECT TO WIFI ======
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ====== CONNECT TO MQTT ======
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(flowPin, INPUT_PULLUP);
  pinMode(leakPin, INPUT);

  attachInterrupt(digitalPinToInterrupt(flowPin), pulseCounter, FALLING);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // ==== MEASURE FLOW ====
  pulseCount = 0;      // Reset count
  delay(1000);         // Count pulses for 1 second
  // Formula: (pulses / 7.5) = liters per minute (for YF-S201)
  flowRate = (pulseCount / 7.5);

  // ==== READ LEAK SENSOR ====
  int leakDetected = digitalRead(leakPin) == LOW ? 1 : 0; // Adjust depending on sensor output

  // ==== SEND DATA TO MQTT ====
  char flowString[10];
  dtostrf(flowRate, 1, 2, flowString); // Convert float to string
  client.publish(flow_topic, flowString);
  client.publish(leak_topic, leakDetected ? "1" : "0");

  // ==== PRINT TO SERIAL ====
  Serial.print("Flow rate: ");
  Serial.print(flowString);
  Serial.println(" L/min");

  Serial.print("Leak: ");
  Serial.println(leakDetected ? "YES" : "NO");

  delay(2000); // Wait before next reading
}
