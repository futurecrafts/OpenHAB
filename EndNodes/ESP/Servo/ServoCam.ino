#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/Imroy/pubsubclient
#include <Servo.h>

const char* ssid = "YourSSID";
const char* password = "YourPassword";

// Update these with values suitable for your network.
IPAddress server(10, 1, 1, 8);

Servo Servo1; // Servo1
Servo Servo2; // Servo2

WiFiClient wclient;
PubSubClient client(wclient, server);

void callback(const MQTT::Publish& pub) {
  // handle message arrived
  if (pub.topic() == "home/esp/node03/servocam/ver")
  {
    // Servo1 on digital pin 9
    Servo1.attach(4);
	
    Serial.print(pub.topic());
    Serial.println(pub.payload_string());
  
    String value = pub.payload_string();
	int intvalue = value.toInt() * 1.8;
  
    Servo1.write(intvalue); // Turn Servo1 position
	delay(1000);          // Wait 1 second
  }
  else if (pub.topic() == "home/esp/node03/servocam/hor")
  {
    // Servo2 on digital pin 10
    Servo2.attach(2);
	Serial.print(pub.topic());
    Serial.println(pub.payload_string());
  
    String value = pub.payload_string();
	int intvalue = value.toInt() * 1.8;
  
    Servo2.write(intvalue); // Turn Servo2 position
	delay(1000);          // Wait 1 second
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // Servo1 on digital pin 9
  //Servo1.attach(4);
  // Servo2 on digital pin 10
  //Servo2.attach(2);
  
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
        if (client.connect("ESPClient03ServoCam")) {
        //client.publish("outTopic","hello world");
        client.subscribe("home/esp/node03/servocam/ver");
        delay(2000);
        client.subscribe("home/esp/node03/servocam/hor");
        }
      }

      if (client.connected())
        client.loop();
    }
}
