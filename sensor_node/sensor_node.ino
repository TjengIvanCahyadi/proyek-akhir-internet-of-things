// Libraries
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <MQ2.h>

#define DHT_PIN D2
#define DHT_TYPE DHT11
#define MQ2_ANALOG_PIN A0

// Update these with values suitable for your network.
const char* ssid = "Universitas Mulawarman";
const char* password = "";

// The mqtt server used
const char* mqtt_server = "broker.hivemq.com";

// Instantiate object from classes
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHT_TYPE);
MQ2 mq2(MQ2_ANALOG_PIN);

// for publishing
#define MSG_BUFFER_SIZE	(1000)
char msg[MSG_BUFFER_SIZE];

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
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);

    setup_wifi();

    client.setServer(mqtt_server, 1883);

    dht.begin();
    mq2.begin();
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    client.loop();

    float temperature = dht.readTemperature();
    float lpgLevel = mq2.readLPG();
    float coLevel = mq2.readCO();
    float smokeLevel = mq2.readSmoke();

    if (isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    delay(3000);

    // publish data
    snprintf (msg, MSG_BUFFER_SIZE, "{\"temp\": %0.2f, \"lpg\": %0.2f, \"co\": %0.2f, \"smoke\": %0.2f}", temperature, lpgLevel, coLevel, smokeLevel);
    client.publish("iot_unmul/iot_c_8/sensor_node", msg);
}