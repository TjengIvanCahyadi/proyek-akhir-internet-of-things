// Libraries
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <BlynkSimpleEsp8266.h>

// BLYNK_AUTH_TOKEN
const char* auth = "i8YwY7uA0M0Wj2CPrqqshdyQkcv4PpoV";

// Update these with values suitable for your network.
const char* ssid = "Universitas Mulawarman";
const char* password = "";

// The mqtt server used
const char* mqttServer = "broker.hivemq.com";

// Instantiate object from classes
WiFiClient espClient;
PubSubClient client(espClient);
BlynkTimer timer;

// for publishing
#define MSG_BUFFER_SIZE	(1000)
char msg[MSG_BUFFER_SIZE];

// variables to store data received from sensor node
float temperature;
float lpgLevel;
float coLevel;
float smokeLevel;

// variables to store data received from alarm node and this data can be changed on Blynk
int alarmForcedState;

/*
    data on this variable will be sent to the alarm node
    data on this variable can be changed from Blynk.
*/
int alarmState = 1;
int ledBrightness = 0;
int buzzerLoudness = 0;

void setupWifi() {
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
            client.subscribe("iot_unmul/iot_c_8/sensor_node");
            client.subscribe("iot_unmul/iot_c_8/alarm_node");
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

    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload, length); //deserialise it

    // check setiap topic yang mungkin
    if (strcmp(topic, "iot_unmul/iot_c_8/sensor_node") == 0) {
        temperature = doc["temp"];
        lpgLevel = doc["lpg"];
        coLevel = doc["co"];
        smokeLevel = doc["smoke"];
    } else if (strcmp(topic, "iot_unmul/iot_c_8/alarm_node") == 0) {
        alarmForcedState = doc["is_alarm_forced_on"];
    }
}

// read data from virtual pin 4 blynk
BLYNK_WRITE(4){
    alarmState = param.asInt();
}

// read data from virtual pin 5 blynk
BLYNK_WRITE(5){
    alarmForcedState = param.asInt();
}

// read data from virtual pin 6 blynk
BLYNK_WRITE(6){
    ledBrightness = param.asInt();
}

// read data from virtual pin 7 blynk
BLYNK_WRITE(7){
    buzzerLoudness = param.asInt();
}

void sendData() {
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, lpgLevel);
    Blynk.virtualWrite(V2, coLevel);
    Blynk.virtualWrite(V3, smokeLevel);
    Blynk.virtualWrite(V5, alarmForcedState);
}

void setup() {
    Serial.begin(115200);
    setupWifi();
    client.setServer(mqttServer, 1883);
    client.setCallback(callback);
    Blynk.begin(auth, ssid, password, "blynk.cloud", 80);
    timer.setInterval(500L, sendData); // send data to Blynk every 500 seconds
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    client.loop();

    Blynk.run();
    timer.run();

    delay(3000);

    // publish data
    snprintf (msg, MSG_BUFFER_SIZE, "{\"is_alarm_on\": %d, \"is_alarm_forced_on\": %d, \"led\": %d, \"buzzer\": %d}", (alarmState && (temperature > 20 || lpgLevel > 30 || coLevel > 30 || smokeLevel > 30)), alarmForcedState, ledBrightness, buzzerLoudness);
    client.publish("iot_unmul/iot_c_8/master_node", msg);
}