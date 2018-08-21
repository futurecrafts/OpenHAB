#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/Imroy/pubsubclient
#include <RCSwitch.h>

const char* ssid = "YourSSID";
const char* password = "YourPassword";

// Update these with values suitable for your network.
IPAddress server(10, 1, 1, 8);

RCSwitch mySwitch = RCSwitch();

WiFiClient wclient;
PubSubClient client(wclient, server);

void callback(const MQTT::Publish& pub) {
  // handle message arrived
  if (pub.topic() == "home/esp/node04/rf433")
  {
    Serial.print(pub.topic());
    Serial.print(pub.payload_string());
  
    String value = pub.payload_string();
  
    if (value == "ON")
    {
      // Same switch as above, but using decimal code
      //mySwitch.send(16221454, 24);
      // Same switch as above, but using binary code
      mySwitch.send("111101111000010100001110");
    }
    else
    {
      // Same switch as above, but using decimal code
      //mySwitch.send(16221446, 24);
      // Same switch as above, but using binary code
      mySwitch.send("111101111000010100000110");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // Transmitter is connected to NodeMCU Pin #2  
  mySwitch.enableTransmit(4);
  
  ///////Connect to WiFi network////////
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
    
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
        if (client.connect("ESPClient04")) {
        //client.publish("outTopic","hello world");
        client.subscribe("home/esp/node04/rf433");
      //delay(5000);
      //client.subscribe("home/rf2");
        }
      }

      if (client.connected())
        client.loop();
    }
}
