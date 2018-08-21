#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/Imroy/pubsubclient

#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 2

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(30, PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "YourSSID";
const char* password = "YourPassword";
int TreeR = 0;
int TreeG = 0;
int TreeB = 0;
boolean finished = true;

// Update these with values suitable for your network.
IPAddress server(10, 1, 1, 8);

WiFiClient wclient;
PubSubClient client(wclient, server);

//void callback(char* topic, byte* payload, unsigned int length) {
void callback(const MQTT::Publish& pub) {
  // handle message arrived
  if (pub.topic() == "home/esp/node02/color")
  {
    if (finished) {
      finished = false;
      Serial.print(pub.topic());
      Serial.print(pub.payload_string());
  
      String value = pub.payload_string();
      // split string at every "," and store in proper variable
      // convert final result to integer
      TreeR = value.substring(0,value.indexOf(',')).toInt();
      TreeG = value.substring(value.indexOf(',')+1,value.lastIndexOf(',')).toInt();
      TreeB = value.substring(value.lastIndexOf(',')+1).toInt();
  
      colorWipe(strip.Color(TreeR, TreeG, TreeB), 300);
    finished = true;
  }
  } 
  else if (pub.topic() == "home/esp/node02/animation")
  {
    if (finished) {
    finished = false;
      Serial.print(pub.topic());
      Serial.print(pub.payload_string());
  
      // There are four fun functions that implement different effects
      String value = pub.payload_string();
      if (value == "0")
      {
        colorWipe(strip.Color(0, 0, 0), 50);
      }
      else if (value == "1")
      {
        colorWipe(strip.Color(255, 0, 0), 50); // Red
      colorWipe(strip.Color(255, 255, 0), 50);
      colorWipe(strip.Color(255, 0, 255), 50);
        colorWipe(strip.Color(0, 255, 0), 50); // Green
      colorWipe(strip.Color(255, 255, 0), 50);
      colorWipe(strip.Color(0, 255, 255), 50);
        colorWipe(strip.Color(0, 0, 255), 50); // Blue
      colorWipe(strip.Color(255, 0, 255), 50);
      colorWipe(strip.Color(0, 255, 255), 50);
      colorWipe(strip.Color(255, 255, 255), 50);
      }
      else if (value == "2")
      {
        theaterChase(strip.Color(127, 127, 127), 50); // White
        theaterChase(strip.Color(127, 0, 0), 50); // Red
        theaterChase(strip.Color(0, 0, 127), 50); // Blue
        theaterChase(strip.Color(0, 127, 0), 50);
        theaterChase(strip.Color(127, 0, 0), 50);
        theaterChase(strip.Color(0, 0, 127), 50);
        theaterChase(strip.Color(0, 127, 0), 50);
      }
      else if (value == "3")
      {
        rainbow(20);
        //rainbowCycle(20);
        //theaterChaseRainbow(50);
      }
      else if (value == "4")
      {
        //rainbow(20);
        rainbowCycle(20);
        //theaterChaseRainbow(50);
      }
    else if (value == "5")
    {
      //rainbow(20);
        //rainbowCycle(20);
        theaterChaseRainbow(50);
    }
    else if (value == "6")
    {
      // Can be extended
    }
    
    finished = true;
  }
  }
}

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
  
  Serial.begin(115200);
  delay(10);
  
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

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
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
        client.subscribe("home/esp/node02/color");
      delay(5000);
      client.subscribe("home/esp/node02/animation");
        }
      }

      if (client.connected())
        client.loop();
    }

  // Some example procedures showing how to display to the pixels:
  //(strip.Color(255, 0, 0), 50); // Red
  //colorWipe(strip.Color(0, 255, 0), 50); // Green
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  // Send a theater pixel chase in...
  //theaterChase(strip.Color(127, 127, 127), 50); // White
  //theaterChase(strip.Color(127, 0, 0), 50); // Red
  //theaterChase(strip.Color(0, 0, 127), 50); // Blue

  //rainbow(20);
  //rainbowCycle(20);
  //theaterChaseRainbow(50);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
