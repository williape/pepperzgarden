// Botanicalls allows plants to ask for human help.
// http://www.botanicalls.com
// original program by Rob Faludi (http://faludi.com) with additional code from various public examples
// Botanicalls is a project with Kati London, Rob Faludi and Kate Hartman
// Pepperz Garden Edition developed by Pete Williams http://github.com/williape/pepperzgarden

#define VERSION "3.5.2" // Pepperz Garden Edition

// Your Token to Tweet (from http://arduino-tweet.appspot.com/)
#define TOKEN "<insert token here>"  
// (the @botanicallstest account token is "14052394-9gYsPnSXTyw0RFVNKMFU14GwNY9RiJXw6Xt3moTkQ")

// external libaries
#include <SPI.h> // library to support SPI bus interactions with Wiznet module
#include <Ethernet.h> // libraries to interact with Wiznet Ethernet
#include <Twitter.h> // library to interact with Twitter  http://www.arduino.cc/playground/Code/TwitterLibrary
#include <EEPROM.h> // library to store information in firmware
#include <TrueRandom.h> // library for better randomization http://code.google.com/p/tinkerit/wiki/TrueRandom

// All messages need to be less than 129 characters
// Arduino RAM is limited. If code fails, try shorter messages
#define URGENT_WATER "Woof, woof. My plants really need water! I'll give them some water."
#define WATER "Woof. You should water my plants."
#define THANK_YOU "Thanks for watering my plants."
#define OVER_WATERED "You over watered my plants."
#define UNDER_WATERED "You didn't water my plants enough."

// sets the soil moisture levels
// moisture is on a scale from 0 to 900
#define MOIST 700 // minimum level of satisfactory moisture
#define DRY 600  // maximum level of tolerable dryness
#define HYSTERESIS 25 // stabilization value http://en.wikipedia.org/wiki/Hysteresis
#define SOAKED 850 // minimum desired level after watering
#define WATERING_CRITERIA 115 // minimum change in value that indicates watering

//sets the watering routine variables
#define PUMP_DURATION 60     // Set the pumping duration - in seconds
#define PUMP_VELOCITY 255    // Set the pump velocity, 245=Medium, 255=High

// sets local network configuration in case of DHCP failure - static settings
IPAddress ip(192,168,1, 220);            // only set if DHCP fails
IPAddress gateway(192,168,1, 1);         // only set if DHCP fails
IPAddress subnet(255, 255, 255, 0);      // only set if DHCP fails

// initialise the Ethernet client library
EthernetClient client;

// tracks the state to avoid erroneously repeated tweets
#define URGENT_SENT 3
#define WATER_SENT 2
#define MOISTURE_OK 1

// set the checking & sampling periods
#define MOIST_SAMPLE_INTERVAL 120 // seconds over which to average moisture samples
#define WATERED_INTERVAL 60 // seconds between checks for watering events
#define TWITTER_INTERVAL 30 // minimum seconds between twitter postings
#define MOIST_SAMPLES 10 //number of moisture samples to average
int moistValues[MOIST_SAMPLES];

// define sampling variables
unsigned long lastMoistTime=0; // storage for millis of the most recent moisture reading
unsigned long lastWaterTime=0; // storage for millis of the most recent watering reading
unsigned long lastTwitterTime=0; // storage for millis of the most recent Twitter message
int lastMoistAvg=500; // storage for moisture value, set at 500 during beta testing period, normally 0
int lastWaterVal=0; // storage for watering detection value

//serial number and counter for tagging posts
unsigned int serial = 0;
unsigned int counter = 0;

// names for the input and output pins
#define MOISTPIN 0 // moisture input is on analog pin 0
#define LEDPIN 7 // generic status LED
#define MOISTLED 6  // LED that indicates the plant needs water
#define COMMLED 5 // LED that indicates communication status
#define PROBEPOWER 4 // feeds power to the moisture probes
#define PUMPPIN 3   //power to pump on digital pin - PWM
#define SWITCH 2// input for normally open momentary switch

// initialize Twitter object
Twitter twitter(TOKEN);

void setup()  { 
  serial = getSerial(); // create or obtain a serial number from EEPROM memory
  counter = getCounter(); // create or obtain a tweet count from EEPROM memory
  
  // set up the serial debugging interface
  Serial.begin(9600);   // set the data rate for the hardware serial port
  Serial.println("");   // begin printing to debug output
  Serial.print("Botanicalls - Pepperz Garden Edition v");
  Serial.println(VERSION);

  // Ethernet Shield Settings
  byte mac[] = {  
    0x02, 0xBC, 0xA1, 0x15, serial >> 8, serial & 0xFF }; // create a private MAC address using serial number
  Serial.print("Device mac: ");
  for (int i=0; i< 6; i++) {
    Serial.print(mac[i],HEX); // print the MAC in a pretty way                             
    if(i<5) Serial.print(":");
  }
  Serial.println("");
  
  // start the Ethernet connection:
  Serial.println("Trying to get an IP address using DHCP");
  if (Ethernet.begin(mac) == 0) {              // initialize the ethernet device using DHCP
    Serial.println("Failed to configure Ethernet using DHCP, using fallback static settings");
     Ethernet.begin(mac, ip, gateway, subnet); // initialize the ethernet device using static settings
  }
  // prints the local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  
  pinMode(LEDPIN, OUTPUT);
  pinMode(PROBEPOWER, OUTPUT);
  pinMode(MOISTLED, OUTPUT);
  pinMode(COMMLED, OUTPUT);
  pinMode(PUMPPIN, OUTPUT);
  pinMode(SWITCH, INPUT);
  digitalWrite(SWITCH, HIGH); // turn on internal pull up resistors
  // initialize moisture value array
  for(int i = 0; i < MOIST_SAMPLES; i++) { 
    moistValues[i] = 0; 
  }
  digitalWrite(PROBEPOWER, HIGH);
  lastWaterVal = analogRead(MOISTPIN);//take a moisture measurement to initialize watering value
  digitalWrite(PROBEPOWER, LOW);

  Serial.print("token: ");
  Serial.println(TOKEN);
  Serial.print("serial: ");
  Serial.println(serial,HEX);
  Serial.print("ctr: ");
  Serial.println(counter,DEC);

    // blink the comm light with the version number
  blinkLED(COMMLED,3,200); // version 3
  delay(200);
  blinkLED(COMMLED,5,200); // point 5
  delay(200);
  blinkLED(COMMLED,1,200); // point 2
  analogWrite(MOISTLED, 36); // turn on the moisture LED
}

void loop()       // main loop of the program     
{
  moistureCheck(); // check to see if moisture levels require Twittering out
  wateringCheck(); // check to see if a watering event has occurred to report it
  buttonCheck(); // check to see if the debugging button is pressed
  analogWrite(COMMLED,0); // douse comm light if it was on

}
