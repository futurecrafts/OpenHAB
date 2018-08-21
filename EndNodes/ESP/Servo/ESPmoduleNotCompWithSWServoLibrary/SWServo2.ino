#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/Imroy/pubsubclient
#include <SoftwareServo.h>

const char* ssid = "YourSSID";
const char* password = "YourPassword";

// Update these with values suitable for your network.
IPAddress server(10, 1, 1, 8);

SoftwareServo Servo1; // Servo1
SoftwareServo Servo2; // Servo2

WiFiClient wclient;
PubSubClient client(wclient, server);

void callback(const MQTT::Publish& pub) {
  // handle message arrived
  if (pub.topic() == "home/esp/node03/servo")
  {
    Serial.println(pub.topic());
    Serial.println(pub.payload_string());
  
    String value = pub.payload_string();
    
	switch(value) {
	  case "0"..."9":
	    Serial.println(value);
		Serial.println(value.toInt()*20);
		Servo1.write(value.toInt()*20);
		Servo2.write(value.toInt()*20);
		break;
	  case "d":
	    Servo1.detach(); // PWM output stop : Servo motor lost power
		Servo2.detach();
		Serial.println("detached!");
		break;
	  case "a":
	    Servo1.attach(14);  // PWM output restart : Servo motor get back power
		Servo2.attach(15);
		Serial.println("attached!");
		break;
	}
	SoftwareServo::refresh(); // keep PWM output continue
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // Servo1 on analog pin 0
  Servo1.attach(14);
  // Servo2 on analog pin 1
  Servo2.attach(15);
  
  Servo1.setMinimumPulse(540); // pulse length for 0 degrees in microsec, 540us default
  Servo1.setMaximumPulse(2400); // pulse length for 180 degrees in microsec, 2400us default
  Servo2.setMinimumPulse(540); // pulse length for 0 degrees in microsec, 540us default
  Servo2.setMaximumPulse(2400); // pulse length for 180 degrees in microsec, 2400us default
  
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
        if (client.connect("ESPClient03SWServo")) {
        //client.publish("outTopic","hello world");
        client.subscribe("home/esp/node03/servo");
      //delay(5000);
      //client.subscribe("home/rf2");
        }
      }

      if (client.connected())
        client.loop();
    }
}
