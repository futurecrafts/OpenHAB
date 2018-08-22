// RFM69 Common node sketch
//
// Current defined devices are:
//
//  0 uptime:     read uptime node in minutes
//  1 node:     read/set transmission interval in seconds, 0 means no periodic transmission
//  2 RSSI:     read radio signal strength
//  3 Version:    read version node software
//  4 voltage:    read battery level
//  5 ACK:      read/set acknowledge message after a 'set' request
//  6 toggle:     read/set toggle function on button press
//  7 timer:      read/set activation timer after button press in seconds, 0 means no timer
//  
//  16  actuator:   read/set LED or relay output
//  40  Button:     tx only: message sent when button pressed
//  48  temperature:  read temperature
//  49  humidity:   read humidity
//  90  error:      tx only: error message if no wireless connection (generated by gateway)
//  92  error:      tx only: device not supported
//  99  wakeup:     tx only: first message sent on node startup
//
//  The button can be set to:
//  - generate a message on each press (limited to one per 10 seconds) and/or
//  - toggle the output ACT1 (local node function) or
//  - activate the output for a fixed time period (local node function)

#include <RFM69.h>
#include <SPI.h>
#include <DHT.h>

//
// CONFIGURATION PARAMETERS
//
#define NODEID 3          // unique node ID within the closed network
#define GATEWAYID 1         // node ID of the Gateway is always 1
#define NETWORKID 100         // network ID of the network
#define ENCRYPTKEY "Yourencryptkeys"       // 16-char encryption key; same as on Gateway!
#define DEBUG           // uncomment for debugging
#define VERSION "RFMNODES V2.1"       // this value can be queried as device 3

// Wireless settings  Match frequency to the hardware version of the radio

//#define FREQUENCY RF69_433MHZ
//#define FREQUENCY RF69_868MHZ
#define FREQUENCY RF69_915MHZ

//#define IS_RFM69HW          // uncomment only for RFM69HW! 
#define ACK_TIME 50           // max # of ms to wait for an ack

//----- SENSOR SETTINGS -----
#define ACT1 9            // Actuator pin (LED or relay)
#define BTN 8           // Button pin
#define SERIAL_BAUD 115200
#define HOLDOFF 2000          // blocking period between button messages

// DHT
#define DHTPIN 8          // DHT data connection
#define DHTTYPE DHT11         // type of sensor

// GasSmoke
int GasSmokeAnalogPin = 0;      // potentiometer wiper (middle terminal) connected to analog pin 
int gas_sensor = -500;           // gas sensor value, current
int gas_sensor_previous = -500;  //sensor value previously sent via RFM

// Flame sensor
int flameAnalogInput = A1;
int flameValue = -50;           //analog value of current flame sensor
int flameValue_previous = -50;  //value previously sent via RFM

// Light sensor
int lightAnalogInput = A2;    //analog input for photo resistor
int lightValue = -50;
int lightValue_previous = -50;

// PIR sensor ================================================
int PirInput = 5;
int PIR_reading = 0;

// Reed Switch ================================================
int ReedInput = 7;
int Reed_reading = 0;
int ReedCount = 0;

// Sound sensor ==============================================
//sound sensor digital input pin
int soundInput = 6;
int sound_reading = 0;  //reading =1 mean no noise, 0=noise

// Ultrasonic
const int pingPin = 6;
const int trigPin = 3;
unsigned long duration;
float inches, cm;
const int ultra_change_amt = 12;      //change amount to notify of change
int ultra_current_value = -200;     //Ultrasonic sensor current value

//
//  STARTUP DEFAULTS
//
long  TXinterval = 20;        // periodic transmission interval in seconds
long  TIMinterval = 20;       // timer interval in seconds
bool  ackButton = false;        // flag for message on button press
bool  toggleOnButton = true;        // toggle output on button press

//
//  VARIABLES
//
long  lastPeriod = -1;        // timestamp last transmission
long  lastBtnPress = -1;        // timestamp last buttonpress
long  lastMinute = -1;        // timestamp last minute
long  upTime = 0;         // uptime in minutes
float hum, temp;          // humidity, temperature
int ACT1State;          // status ACT1 output
int signalStrength;         // radio signal strength
bool  setAck = false;         // send ACK message on 'SET' request
bool  send0, send1, send2, send3, send4;
bool  send5, send6, send7;
bool  send16, send40, send48, send49, send92;   // message triggers
bool    send41, send42, send43, send50, send51, send52, send53;
bool  send64;
bool  promiscuousMode = false;      // only listen to nodes within the closed network
bool  curState = true;        // current button state
bool  lastState = true;       // last button state
bool  wakeUp = true;          // wakeup flag
bool  timerOnButton = false;        // timer output on button press
bool  msgBlock = false;       // flag to hold button messages to prevent overload

typedef struct {          // Radio packet format
int nodeID;           // node identifier
int devID;            // device identifier 
int cmd;            // read or write
int cmd2;
long  intVal;           // integer payload
float fltVal;           // floating payload
char  payLoad[32];          // string payload
} Message;

Message mes;

DHT dht(DHTPIN, DHTTYPE, 3);      // initialise temp/humidity sensor for 3.3 Volt arduino
RFM69 radio;

//
//=====================   SETUP ========================================
//
void setup() {
#ifdef DEBUG
  Serial.begin(SERIAL_BAUD);
#endif

pinMode(ACT1, OUTPUT);          // set actuator 1
ACT1State = 0;
digitalWrite(ACT1, ACT1State);
dht.begin();            // initialise temp/hum sensor
pinMode(soundInput, INPUT);         // initialise sound/noise sensor
pinMode(PirInput, INPUT);         // initialise PIR sensor
pinMode(pingPin, INPUT);      // Ultrasonic
pinMode(trigPin, OUTPUT);     // Ultrasonic
pinMode(ReedInput, INPUT);      // Reed
//pinMode(4, OUTPUT);
radio.initialize(FREQUENCY,NODEID,NETWORKID);   // initialise radio 
#ifdef IS_RFM69HW
radio.setHighPower();           // only for RFM69HW!
#endif
radio.encrypt(ENCRYPTKEY);        // set radio encryption 
radio.promiscuous(promiscuousMode);     // only listen to closed network
wakeUp = true;            // send wakeup message

#ifdef DEBUG
  Serial.print("Node Software Version ");
  Serial.println(VERSION);
  Serial.print("\nTransmitting at ");
  Serial.print(FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(" Mhz...");
#endif
} // end setup

//
//
//====================    MAIN  ========================================
//
void loop() {
// RECEIVE radio input
//
if (receiveData()) parseCmd();        // receive and parse any radio input

// DETECT INPUT CHANGE
//
/*curState = digitalRead(BTN);
msgBlock = ((millis() - lastBtnPress) < HOLDOFF);   // hold-off time for additional button messages
if (!msgBlock &&  (curState != lastState)) {      // input changed ?
  delay(5);
  lastBtnPress = millis();              // take timestamp
  send40 = true;                    // set button message flag
  if (curState == LOW) {
  if (toggleOnButton) {               // button in toggle state ?
  ACT1State = !ACT1State;               // toggle output
  digitalWrite(ACT1, ACT1State);
  } else
  if (TIMinterval > 0 && !timerOnButton) {      // button in timer state ?
  timerOnButton = true;               // start timer interval
  ACT1State = HIGH;                 // switch on ACT1
  digitalWrite(ACT1, ACT1State);
  }}
lastState = curState;
}

// TIMER CHECK
//
if (TIMinterval > 0 && timerOnButton)     // =0 means no timer
{ 
  if ( millis() - lastBtnPress > TIMinterval*1000) {  // timer expired ?
  timerOnButton = false;        // then end timer interval 
  ACT1State = LOW;        // and switch off Actuator
  digitalWrite(ACT1, ACT1State);
  }
}*/

// UPTIME 
//
if (lastMinute != (millis()/60000)) {     // another minute passed ?
  lastMinute = millis()/60000;
  upTime++;
  }

// PERIODIC TRANSMISSION
//
if (TXinterval > 0)
{
int currPeriod = millis()/(TXinterval*1000);
if (currPeriod != lastPeriod) {       // interval elapsed ?
  lastPeriod = currPeriod;
  
// list of sensordata to be sent periodically..
// remove comment to include parameter in transmission
//  send1 = true;         // send transmission interval
//  send2 = true;           // signal strength
//  send3 = true;           // Version
//  send4 = true;         // voltage level
//  send5 = true;         // Acknowledge
//  send6 = true;         // Toggle
//  send7 = true;         // timer
//  send16 = true;          // actuator state
  //send40 = true;          // Button
  //send41 = true;          // PIR
  //send42 = true;          // Sound
  //send43 = true;          // Reed
  //send48 = true;          // send temperature
  //send49 = true;          // send humidity
  send50 = true;          // Gas,Smoke
  //send51 = true;          // Flame
  //send52 = true;          // Light
  //send53 = true;          // Ultrasonic
  //send64 = true;          // Reed Count
  }
}

// SEND RADIO PACKETS
//
sendMsg();            // send any radio messages 

}   // end loop

//
//
//=====================   FUNCTIONS ==========================================

//
//========    RECEIVEDATA : receive data from gateway over radio
//

bool receiveData() {
bool validPacket = false;
if (radio.receiveDone())        // check for received packets
{
if (radio.DATALEN != sizeof(mes))     // wrong message size means trouble
#ifdef DEBUG
  Serial.println("invalid message structure..")
#endif
;
else
{
  mes = *(Message*)radio.DATA;
  validPacket = true;       // YES, we have a packet !
  signalStrength = radio.RSSI;
#ifdef DEBUG
  Serial.print(mes.devID);
  Serial.print(", ");
  Serial.print(mes.cmd);
  Serial.print(", ");
  Serial.print(mes.intVal);
  Serial.print(", ");
  Serial.print(mes.fltVal);
  Serial.print(", RSSI= ");
  Serial.println(radio.RSSI);
  Serial.print("Node: ");
  Serial.println(mes.nodeID);
#endif  
}
}
if (radio.ACKRequested()) radio.sendACK();    // respond to any ACK request
return validPacket;         // return code indicates packet received
}   // end recieveData

//
//
//==============    PARSECMD: analyse messages and execute commands received from gateway
//

void parseCmd() {         // parse messages received from the gateway
send0 = false;            // initialise all send triggers
send1 = false;
send2 = false;
send3 = false; 
send4 = false;
send5 = false;
send6 = false;
send7 = false;
send16 = false;
send40 = false;
send41 = false;
send42 = false;
send43 = false;
send48 = false;
send49 = false;
send50 = false;
send51 = false;
send52 = false;
send53 = false;
send64 = false;
send92 = false;

switch (mes.devID)          // devID indicates device (sensor) type
{
case (0):           // uptime
if (mes.cmd == 1) send0 = true;
break;
case (1):           // polling interval in seconds
if (mes.cmd == 0) {         // cmd == 0 means write a value
  TXinterval = mes.intVal;      // change interval to radio packet value
  if (TXinterval <10 && TXinterval !=0) TXinterval = 10;  // minimum interval is 10 seconds
  if (setAck) send1 = true;     // send message if required
#ifdef DEBUG
  Serial.print("Setting interval to ");
  Serial.print(TXinterval);
  Serial.println(" seconds");
#endif
}
else send1 = true;          // cmd == 1 is a read request, so send polling interval 
break;
case (2):             // signal strength
if (mes.cmd == 1) send2 = true;
break;
case (3):             // software version
if (mes.cmd == 1) send3 = true;
break;
case (4):             // battery level
if (mes.cmd == 1) send4 = true;
break;
case (5):             // set ack status
if (mes.cmd == 0) {
  if (mes.intVal == 0) setAck = false;
  if (mes.intVal == 1) setAck = true;
  if (setAck) send5 = true;     // acknowledge message ?
}
else send5 = true;          // read request means schedule a message
break;
case (6):             // set toggle
if (mes.cmd == 0) {
  if (mes.intVal == 0) toggleOnButton = false;
  if (mes.intVal == 1) toggleOnButton = true;
  if (setAck) send6 = true;     // acknowledge message ?
}
else send6 = true;
break;
case (7):           // timer interval in seconds
if (mes.cmd == 0) {         // cmd == 0 means write a value
  TIMinterval = mes.intVal;     // change interval 
  if (TIMinterval <5 && TIMinterval !=0) TIMinterval = 5;
  if (setAck) send7 = true;     // acknowledge message ?
}             // cmd == 1 means read a value
else send7 = true;          // send timing interval 
break;
case (16):            // Actuator 1
if (mes.cmd == 0) {         // cmd == 0 means write
  if(mes.intVal == 0 || mes.intVal == 1) {
  ACT1State = mes.intVal;
  digitalWrite(ACT1, ACT1State);
  if (setAck) send16 = true;      // acknowledge message ?
#ifdef DEBUG  
  Serial.print("Set LED to ");
  Serial.println(ACT1State);
#endif
}}
else send16 = true;         // cmd == 1 means read
break;
case (40):            // binary input
if (mes.cmd == 1) send40 = true;
break;
case (41):            // PIR
if (mes.cmd == 1) send41 = true;
break;
case (42):            // Sound
if (mes.cmd == 1) send42 = true;
break;
case (43):            // Reed
if (mes.cmd == 1) send43 = true;
break;
case (48):            // temperature
if (mes.cmd == 1) send48 = true;
break;
case (49):            // humidity
if (mes.cmd == 1) send49 = true;
break;
case (50):            // Gas,Smoke
if (mes.cmd == 1) send50 = true;
break;
case (51):            // Flame
if (mes.cmd == 1) send51 = true;
break;
case (52):            // Light
if (mes.cmd == 1) send52 = true;
break;
case (53):            // Ultrasonic
if (mes.cmd == 1) send53 = true;
break;
case (64):            // Reed Count
if (mes.cmd == 1) send64 = true;
break;
default: send92 = true;         // no valid device parsed
}
} // end parseCmd

//
//
//======================    SENDMSG: sends messages that are flagged for transmission
//

void sendMsg() {          // prepares values to be transmitted
bool tx = false;          // transmission flag
mes.nodeID=NODEID;
mes.intVal = 0;
mes.fltVal = 0;
mes.cmd = 0;            // '0' means no action needed in gateway
int i;
for ( i = 0; i < sizeof(VERSION); i++){
mes.payLoad[i] = VERSION[i];  }
mes.payLoad[i] = '\0';          // software version in payload string

if (wakeUp) {           // send wakeUp call 
  mes.devID = 99; 
  wakeUp = false;         // reset transmission flag for this message
  txRadio();          // transmit
}
if (send0) {
  mes.devID = 0;
  mes.intVal = upTime;        // minutes uptime
  send0 = false;
  txRadio();
}
if (send1) {            // transmission interval
  mes.devID = 1;
  mes.intVal = TXinterval;      // seconds (integer)
  send1 = false;
  txRadio();
}
if (send2) {
  mes.devID = 2;
  mes.intVal = signalStrength;      // signal strength (integer)
  send2 = false;
  txRadio();
}
if (send3) {            // node software version (string)
  mes.devID = 3;          // already stored in payload string
  send3 = false;
  txRadio();
}
if (send4) {            // measure voltage..
  mes.devID = 4;  
  long result;          // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);         // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);        // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result;       // Back-calculate in mV
  mes.fltVal = float(result/1000.0);    // Voltage in Volt (float)
  txRadio();
  send4 = false;
}
if (send5) {            // Acknowledge on 'SET'
  mes.devID = 5;
  if (setAck) mes.intVal = 1; else mes.intVal = 0;// state (integer)
  send5 = false;
  txRadio();
}
if (send6) {            // Toggle on Buttonpress 
  mes.devID = 6;
  if (toggleOnButton) mes.intVal = 1;     // read state of toggle flag
  else mes.intVal = 0;        // state (integer)
  send6 = false;
  txRadio();
}
if (send7) {            // timer interval
  mes.devID = 7;
  mes.intVal = TIMinterval;     // seconds (integer)
  send7 = false;
  txRadio();
}
if (send16) {           // state of Actuator 1
  mes.devID = 16;
  mes.intVal = ACT1State;       // state (integer)
  send16 = false;
  txRadio();
}
if (send40) {           // Binary input read
  mes.devID = 40;
  if (curState == LOW) mes.intVal = 1;          // state (integer)
  send40 = false;
  txRadio();
}
if (send41) {           // PIR
  mes.devID = 41;
  //1 mean presence detected
  PIR_reading = digitalRead(PirInput);
#ifdef DEBUG  
  Serial.print("PIR value is : ");
  Serial.println(PIR_reading);
#endif
  //if (PIR_reading == 1) {
  //  mes.intVal = 1;         // state (integer)
    mes.intVal = PIR_reading;
    send41 = false;
    txRadio();
  //}
}
if (send42) {           // Sound
  mes.devID = 42;
  // 1 = no noise, 0 = noise!!
  sound_reading = digitalRead(soundInput);
#ifdef DEBUG  
  Serial.print("Sound value is : ");
  Serial.println(sound_reading);
#endif
  if (sound_reading == 0) {
    mes.intVal = 0;         // state (integer)
  }
  else
  {
    mes.intVal = 1;         // state (integer)
  }
  send42 = false;
  txRadio();
}
if (send43) {           // Reed
  mes.devID = 43;
  //1 mean close
  Reed_reading = digitalRead(ReedInput);
#ifdef DEBUG  
  Serial.print("Reed value is : ");
  Serial.println(Reed_reading);
#endif
  //if (Reed_reading == 1) {
  //  mes.intVal = 1;         // state (integer)
    mes.intVal = Reed_reading;
    send43 = false;
    txRadio();
 // }
}
if (send48) {           // Temperature
  mes.devID = 48;
  temp = dht.readTemperature();
  mes.fltVal = temp;        // Degrees Celcius (float)
  send48 = false;
  txRadio();
}
if (send49) {           // Humidity
  mes.devID = 49;
  hum = dht.readHumidity();
  mes.fltVal = hum;       // Percentage (float)
  send49 = false;
  txRadio();
}
if (send50) {           // Gas,Smoke
  mes.devID = 50;
  // don't read analog pins too often (<1Hz), else caps never get to charge.
    //112 to 120 = normal, 400 = high
  gas_sensor = analogRead(GasSmokeAnalogPin);
#ifdef DEBUG  
  Serial.print("Current Gas is : ");
  Serial.println(gas_sensor);
#endif
  //if ((gas_sensor < (gas_sensor_previous - 70)) || (gas_sensor > (gas_sensor_previous + 70)))
 // {
    mes.fltVal = gas_sensor;        // float
    gas_sensor_previous = gas_sensor;
    send50 = false;
    txRadio();
 // }
  //send50 = false;
}
if (send51) {           // Flame
  mes.devID = 51;
  //analog value:  usually 1023 for no fire, lower for fire.
  flameValue = analogRead(flameAnalogInput);
#ifdef DEBUG  
  Serial.print("Abnormal Flame detected : ");
  Serial.println(flameValue);
#endif
  //if ((flameValue < (flameValue_previous - 20)) || (flameValue > (flameValue_previous + 20)))
   // {
    mes.fltVal = flameValue;        // float
    flameValue_previous = flameValue;
    send51 = false;
    txRadio();
  //}
}
if (send52) {           // Light
  mes.devID = 52;
  //analog value:  Less than 100 is dark.  greater than 500 is room lighting
  lightValue = analogRead(lightAnalogInput);
#ifdef DEBUG  
  Serial.print("Light value is : ");
  Serial.println(lightValue);
#endif
  //if ((lightValue < (lightValue_previous - 50)) || (lightValue > (lightValue_previous + 100)))
  //  {
    mes.fltVal = lightValue;        // float
    lightValue_previous = lightValue;
    send52 = false;
    txRadio();
 // }
}
if (send53) {           // Ultrasonic
  mes.devID = 53;
  // get data
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(15);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(pingPin, HIGH);
  // convert the time into a distance
  inches = duration / 74.0 / 2.0;
  cm = duration / 29 / 2;
#ifdef DEBUG  
  Serial.print("inches : ");
  Serial.println(inches);
  Serial.print("cm : ");
  Serial.println(cm);
#endif
  //if ((inches < (ultra_current_value - ultra_change_amt)) || (inches > (ultra_current_value + ultra_change_amt)))
    //{
    mes.fltVal = cm;        // float
    ultra_current_value = cm;
    send53 = false;
    txRadio();
  //}
}
if (send64) {           // Reed Count
  mes.devID = 64;
  //1 mean close
#ifdef DEBUG  
  Serial.print("Reed Count value is : ");
  Serial.println(ReedCount);
#endif
  if (Reed_reading == 1) {
  //  mes.intVal = 1;         // state (integer)
    mes.intVal = ReedCount;
    ReedCount = ReedCount + 1;
    send64 = false;
    txRadio();
  }
  send64 = false;
}
if (send92) {           // error message invalid device
  mes.intVal = mes.devID;
  mes.devID = 92;
        send92 = false;
  txRadio();
}

}
//
//
//=======================   TXRADIO
//

void txRadio()            // Transmits the 'mes'-struct to the gateway
{
  Serial.print("size : ");
    Serial.println(sizeof(mes));
if (radio.sendWithRetry(GATEWAYID, (const void*)(&mes), sizeof(mes)))
#ifdef DEBUG
  {Serial.print(" message ");
  Serial.print(mes.devID);
  Serial.println(" sent...");}
  else Serial.println("No connection...")
#endif
;}  // end txRadio
