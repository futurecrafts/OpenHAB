/*
  Servotester

  A servo is connected to pin 14 and is sweeping much like the arduino example.
  This has an arduino-type library for controlling servos. Ticker is used to drive the servo pins
*/
#define servopin 2
#include <Ticker.h>
#include "myServo.h"

Servo servo;

void setup() {
  servo.attach(servopin);
  Serial.begin(115200);
  delay(10);
}

int pos;
void loop() {

  //Serial.println("servo.write(0) ");
  //servo.write(0);            // tell servo to go to position in variable 'pos'
  //delay(5000);   
  //Serial.println("servo.write(45) ");
  //servo.write(45);            // tell servo to go to position in variable 'pos'
  //delay(5000); 
  //Serial.println("servo.write(90) ");
  //servo.write(90);            // tell servo to go to position in variable 'pos'
  //delay(5000);
  //Serial.println("servo.write(180) ");
  //servo.write(180);            // tell servo to go to position in variable 'pos'
  //delay(5000);
  //Serial.println("servo.write(90) ");
  //servo.write(90);            // tell servo to go to position in variable 'pos'
  //delay(5000);
  delay(5000);
  Serial.println("start... ");
  for (pos = 0; pos <= 45; pos += 5) // goes from 0 degrees to 180 degrees
  { // in steps of 1 degree
    servo.write(pos);            // tell servo to go to position in variable 'pos'
    delay(20);                       // waits 15ms for the servo to reach the position
  }
  delay(5000);
  Serial.println("servo.write(0) ");
  servo.write(0);            // tell servo to go to position in variable 'pos'
  delay(5000);

}
////////////////////////////////////////////////////////////////////////////
