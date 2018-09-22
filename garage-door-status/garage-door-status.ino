#include <PubSubClient.h>

#include <FS.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <SimpleTimer.h>

#define mqtt_server       "192.168.1.107"
#define mqtt_port         "1883"
#define mqtt_user         "ashishp"
#define mqtt_pass         "ashishp"
#define door_status_topic    "home/garage/door/status"

SimpleTimer timer;
WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;
ESP8266WebServer server(80);
// WiFiUDP ntpUDP;

long lastMsg = 0;
int16_t utc = -8; //UTC -7:00 PST
uint32_t currentMillis = 0;
uint32_t previousMillis = 0;
long SECONDS = 1000000;
long DEEP_SLEEP_TIME = 1 * SECONDS;

int lastValue = LOW;
bool isFirstMessage = true;

// NTPClient timeClient(ntpUDP, "time-a.nist.gov", utc*3600, 60000);

const int INPUT_PIN = 2;

void setupWifiManager() {
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT Username", mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", mqtt_pass, 20);

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);

  IPAddress _ip = IPAddress(192, 168, 1, 150);
  IPAddress _gw = IPAddress(192, 168, 1, 1);
  IPAddress _sn = IPAddress(255, 255, 255, 0);

  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

  //tries to connect to last known settings
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP" with password "password"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("garage-door-AP", "default")) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected to wifi...");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  //read updated parameters
  Serial.println("connected to wifi...Getting MQTT Server Configuration");
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());

  int mqtt_port_x = String(mqtt_port).toInt();
  Serial.println(mqtt_server);
  Serial.println(mqtt_port_x);
  Serial.println(mqtt_user);
  Serial.println(mqtt_port);
 
  client.setServer(mqtt_server, mqtt_port_x);
  Serial.print("Connected to MQTT Server....");
}

void reconnectMQTT() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.publish("home/garage/door/sensor/status", "connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setupWebserver() {
  Serial.println("Starting Web Server");
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/reset", []() {
    server.send(200, "text/plain", "Resetting Wifi Settings");
    wifiManager.resetSettings();
    ESP.reset();
  });

  server.on("/door/status", handleStatusQuery);

  server.begin();
  Serial.println("HTTP server started");
}

void handleStatusQuery() {
  int value = digitalRead(INPUT_PIN);
  if (value == LOW) {
    server.send(200, "text/plain", "{" \
      "\"device_type\": \"obstacle_detector\", " \
      "\"has_obstacle\": 1," \
      "\"location\": \"garage\"" \
    "}");
  } else  {
    server.send(200, "text/plain", "{" \
      "\"device_type\": \"obstacle_detector\", " \
      "\"has_obstacle\": 0," \
      "\"location\": \"garage\"" \
    "}");
  }
}

void checkForStatus() {
    // Listen for requests
  // server.handleClient();

  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  long now = millis();
  // if (now - lastMsg > 1000) {
    lastMsg = now;

    int value = digitalRead(INPUT_PIN);

    if (isFirstMessage || lastValue != value) {
      String hasObstacle;
      if (value == LOW) {
        hasObstacle = "Close";
      } else {
        hasObstacle = "Open";
      }
  
      Serial.print("New Status:");
      Serial.println(String(hasObstacle).c_str());
      client.publish(door_status_topic, String(hasObstacle).c_str(), true);
      isFirstMessage = false;
      lastValue = value;
    }
  // }

  Serial.println("ESP Going into Sleep Mode");
  ESP.deepSleep(5 * SECONDS); // DEEP_SLEEP_TIME);
  delay(100);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
  Serial.println("");

  setupWifiManager();
  setupWebserver();

   timer.setInterval(1000L, checkForStatus);

  // Serial.println("ESP Going into Sleep Mode");
  // ESP.deepSleep(5 * SECONDS); // DEEP_SLEEP_TIME);
}

void loop() {
  timer.run();
}

