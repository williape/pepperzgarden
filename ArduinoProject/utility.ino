
// post a tweet
void posttweet(char* msg) {
  digitalWrite(COMMLED,HIGH); // light the Communications LED
  // assemble a string for Twitter, appending a unique ID to prevent Twitter's repeat detection
  char *str1 = " [";
  char *str2; 
  str2= (char*) calloc (5,sizeof(char)); // allocate memory to string 2
  itoa(serial,str2,16); // turn serial number into a string
  char *str3 = "-";
  char *str4;
  str4= (char*) calloc (5,sizeof(char)); // allocate memory to string 4
  itoa(counter%10000,str4,10); // turn message counter into a string
  char *str5 = "]";
  char *message;
  // allocate memory for the message
  message = (char*) calloc(strlen(msg) + strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4) + strlen(str5) + 1, sizeof(char));
  strcat(message, msg); // assemble (concatenate) the strings into a message
  strcat(message, str1);
  strcat(message, str2);   
  strcat(message, str3);
  strcat(message, str4);
  strcat(message, str5);
  Serial.println("connect...");
  if (twitter.post(message)) { // attempt to tweet the message
      int status = twitter.wait(); // receive the status
      digitalWrite(COMMLED,LOW); // turn off the communications LED
      delay(100);
      if (status == 200) {
        Serial.println("tweet ok");
      } 
      else {
        Serial.print("tweet fail: code ");
        Serial.println(status); // if tweet fails, print the error code
        blinkLED(COMMLED,2,100); // ...and blink the communications LED twice
      }
      counter++; // iterate the message counter
      setCounter(counter); // store the message counter in EEPROM memory
    } 
    else {
      Serial.println("connect fail"); // if connection to Twitter fails,
      blinkLED(COMMLED,4,100); // ...blink the communications LED 4 times
    } 
  free(message); // free the allocated string memory
  free(str2);
  free(str4);
}

// water pumping routine 
void pumpWater() {
      analogWrite(PUMPPIN, PUMP_VELOCITY); // turn on the pump
      digitalWrite(LEDPIN, HIGH);
      Serial.println("Pumping water");
      delay(PUMP_DURATION*1000);  // pumping duration
      analogWrite(PUMPPIN, 0); // turn off the pump
      digitalWrite(LEDPIN, LOW);
      Serial.println("Done pumping water");
}

// retrieve the randomized unit serial number information from EEPROM
unsigned int getSerial() {
  unsigned int ser ;
  if (EEPROM.read(2) != 1) {
    Serial.println("init ser");
    ser = TrueRandom.random(1,0xFFFE);
    EEPROM.write(0,ser >> 8);
    EEPROM.write(1,ser & 0xFF);
    EEPROM.write(2,1);
  }
  ser = (EEPROM.read(0) << 8) + (EEPROM.read(1));
  return ser;
}

// retrieve the message counter information from EEPROM
unsigned int getCounter() {
  unsigned int ctr;
  //initial setting of counter
  if (EEPROM.read(5) != 1) { // if counter set status is false
    Serial.println("init ctr");
    EEPROM.write(3,0); // write LEB zero
    EEPROM.write(4,0); // write MSB zero
    EEPROM.write(5,1); // counter set status is true
  }
  //get counter reading
  ctr = (EEPROM.read(3) << 8) + (EEPROM.read(4)); // add MSB + LSB for 16-bit counter
  return ctr;
}

// write the message counter information to EEPROM
void setCounter(unsigned int ctr) {
  EEPROM.write(3,ctr >> 8); // write the MSB
  EEPROM.write(4,ctr & 0xFF); // write the LSB
}

// this function blinks an LED light as many times as requested
void blinkLED(byte targetPin, int numBlinks, int blinkRate) {
  for (int i=0; i<numBlinks; i++) {
    digitalWrite(targetPin, HIGH);   // sets the LED on
    delay(blinkRate);                // waits for a blinkRate milliseconds
    digitalWrite(targetPin, LOW);    // sets the LED off
    delay(blinkRate);
  }
}

