// Libraries
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define LED_PIN D2
#define BUZZER_PIN D1

// Update these with values suitable for your network.
const char* ssid = "Universitas Mulawarman";
const char* password = "";

// The mqtt server used
const char* mqtt_server = "broker.hivemq.com";

// Instantiate object from classes
WiFiClient espClient;
PubSubClient client(espClient);

// for publishing
#define MSG_BUFFER_SIZE	(1000)
char msg[MSG_BUFFER_SIZE];

// variables to store data received from master node
int isAlarmOn;
int isAlarmForcedOn;
int ledBrightness;
int buzzerLoudness;

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

// connect to the broker
void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            client.subscribe("iot_unmul/iot_c_8/master_node");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

// called when there is a new message from a subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Pesan Diterima [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    StaticJsonDocument<1024> doc; //read JSON data
    deserializeJson(doc, payload, length); //deserialise it

    isAlarmOn = doc["is_alarm_on"];
    isAlarmForcedOn = doc["is_alarm_forced_on"];
    ledBrightness = doc["led"];
    buzzerLoudness = doc["buzzer"];
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    setup_wifi();

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    client.loop();

    // Event log
    if (isAlarmForcedOn || isAlarmOn) {
        // nyalakan alarm
        analogWrite(LED_PIN, ledBrightness);
        analogWrite(BUZZER_PIN, buzzerLoudness);
    } else {
        // matikan alarm
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
    }

    delay(3000);

    // publish data
    snprintf (msg, MSG_BUFFER_SIZE, "{\"is_alarm_forced_on\": %d}", isAlarmForcedOn);
    client.publish("iot_unmul/iot_c_8/alarm_node", msg);
}