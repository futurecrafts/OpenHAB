#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/Imroy/pubsubclient

const char* ssid = "YourSSID";
const char* password = "YourPassword";

// Update these with values suitable for your network.
IPAddress server(10, 1, 1, 8);

WiFiClient wclient;
PubSubClient client(wclient, server);

void callback(const MQTT::Publish& pub) {
  // handle message arrived
  if (pub.topic() == "home/esp/node01/ledcandle")
  {
    Serial.print(pub.topic());
    Serial.print(pub.payload_string());
  
    String value = pub.payload_string();
  
    if (value == "ON")
    {
      digitalWrite(0, HIGH);
      Serial.print("Now ON");
    }
    else
    {
      digitalWrite(0, LOW);
      Serial.print("Now OFF");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  ///////Connect to WiFi network////////
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  pinMode(0, OUTPUT);
    
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Print the IP address
  Serial.println(WiFi.localIP());
  ///////////////////////////////////////
  
  //////////////Mqtt Setup///////////////
  client.set_callback(callback);
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Connecting to ");
      Serial.print(ssid);
      Serial.println("...");
      WiFi.begin(ssid, password);

      if (WiFi.waitForConnectResult() != WL_CONNECTED)
        return;
      Serial.println("WiFi connected");
    }
  
  if (WiFi.status() == WL_CONNECTED) {
      if (!client.connected()) {
        if (client.connect("ESPClient01")) {
        //client.publish("outTopic","hello world");
        client.subscribe("home/esp/node01/ledcandle");
      //delay(5000);
      //client.subscribe("home/rf2");
        }
      }

      if (client.connected())
        client.loop();
    }
}
