//----[ OutGaugeBus ]------------------------------------------------------
//  Name    : OGBUS.PDE
//-------------------------------------------------------------------------
//  Purpose : Receive serial commmands from "Live For Speed" INSIM OUTGAUGE
//          : Client and then display real-time dashboard information
//          : on Arduino based output devices, such as LED's.
//          : Live For Speed References:
//          : LFS/DOCS/INSIM.TXT
//          : http://en.lfsmanual.net/wiki/InSim_Tutorials
//          : http://www.brunsware.de/insim/
//          : See Also: http://www.lfs.net/
//-------------------------------------------------------------------------
//  Date    : 25 Oct, 2007 ( Started as a Parallax SX28 project )
//  Version : 2.11
//  Modified: 12/22/2010 11:30:34 AM
//  Author  : Pete Willard
//          :
//Additional:
//  Credits : * Mike McRoberts - Earthshine Design -
//          :     Arduino Starter Kit PDF
//          :
//          : * Hacktronics - http://www.hacktronics.com/Tutorials
//          :
//          : * Arduino ShiftOut tutorial
//          :
//          : To keep code "recognizable", available references were used
//          : and changed very little from reference sources listed above.
//-------------------------------------------------------------------------
//  Notes   : Includes using a 74HC595 Shift Register for the Bar Graph
//          : and discrete 7-segment display for gear indicator
//          :
//          : Commands come from OGBUS.EXE (DevC++ UDP Client) and convert
//          : INSIM OUTGAUGE data into serial packet data for Arduino
//          : INSIM Client Software will send ALL vaalues every update
//          : but can send as little as Clear Command "C0"
//          :
//          : Expansion:
//          : With a little creative wiring, it would be possible to
//          : daisy chain multiple serial Arduino boards or if USB is
//          : used, multiple OGBUS clients can be run at one time
//          :
//          : Command Examples:
//          : (Comma Separated Values) Any Order up to 24 Characters
//          : Format: <command><argument>,<command><argument>...<cr><lf>
//          : C0    = Clear All Outputs
//          : G0-G8 = Gear Indicator
//          : R0-R8 = RPM Bargraph
//          : S0|S1 = Shift Light Pin On|Off (binary)
//          : P0|P1 = Pit Limiter Pin On|Off (binary)
//          :
//-------------------------------------------------------------------------


//----[ Variables ]--------------------------------------------------------
const int buffsize = 25;     // Buffer Size
char buffer[buffsize];      // Incoming Serial Data Buffer
int  debug    = 0;          // Set to NON-ZERO to test with Serial Monitor
int  ledCount = 8;          // The number of LEDs in the Bar Graph LED
//-------------------------------------------------------------------------
// Seven segment LED layout for the Gear Indicator
// Arduino pins must be sequential
int GIstartpin = 2;
//                    Arduino pin: 2,3,4,5,6,7,8
byte seven_seg_digits[9][7] = {  { 0,0,0,0,1,0,1 },  // = R
                                 { 0,0,1,0,1,0,1 },  // = N
                                 { 0,1,1,0,0,0,0 },  // = 1
                                 { 1,1,0,1,1,0,1 },  // = 2
                                 { 1,1,1,1,0,0,1 },  // = 3
                                 { 0,1,1,0,0,1,1 },  // = 4
                                 { 1,0,1,1,0,1,1 },  // = 5
                                 { 1,0,1,1,1,1,1 },  // = 6
                                 { 0,0,0,0,0,0,0 },  // = 7 (blank)
                                                 };
//                                 a b c d e f g ------>  LED segment
//-------------------------------------------------------------------------
// Bargraph Values for 8 LED Bargraph
// Only 8 values of BYTE are needed to light the 8 bargraph LED's
// sequentially
// NOTE: Most Bargraph LED's are "10 unit" so I have bottom 2 and top 2
// LED's tied together.
// Part Number used:   AVAGO "HDSP-4832"  3-Green 4-Yellow 3-Red
// Mouser Part Number: 630-HDSP-4832
// The Shift Register lowest outputs start with Green Anodes of the
// Bargraph.
//
byte bargraph[9] = {0x00,0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF};
//-------------------------------------------------------------------------
// LED PINS - optional since we are low on pins
int shiftlight = 12;
//int pitlimit   = 13;     // Pick one
//int lowfuel    = 13;
//-------------------------------------------------------------------------
// 74HC595 Pin Setup
int latchPin = 9;   // Pin connected to ST_CP (12) of 74HC595
int clockPin = 10;  // Pin connected to SH_CP (11) of 74HC595
int dataPin  = 11;  // Pin connected to DS    (14) of 74HC595
long rpmTime = 0;
long speedTime = 0;
long rpm = 1200;
long speedo = 1200;
int rpmState = LOW;             
int speedState = LOW;
long previousMillis = 0;        // will store last time LED was updated
long previousMillis2 = 0;        // will store last time LED was updated
//----[ SETUP ]-----------------------------------------------------------
void setup() {
  // Speed needs to match INSIM - Outgauge Client Configuration setting
  Serial.begin(115200);
  Serial.flush();

  // Set all pins to Outut Mode
  int a;
  //for(a=0;a < 13;a++){
  //    pinMode(a, OUTPUT);
  // }
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  pinMode(6, OUTPUT); //RPM
  pinMode(7, OUTPUT); //Speed
  
}

//----[ MAIN LOOP ]------------------------------------------------------
void loop() {
   unsigned long currentMillis = micros();
       
  if(currentMillis - previousMillis > rpmTime) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (rpmState == LOW)
      rpmState = HIGH;
    else
      rpmState = LOW;

    // set the LED with the ledState of the variable:
    digitalWrite(6, rpmState);
  }
  
  unsigned long currentMillis2 = micros();
  if(currentMillis2 - previousMillis2 > speedTime) {
    // save the last time you blinked the LED 
    previousMillis2 = currentMillis2;   

    // if the LED is off turn it on and vice-versa:
    if (speedState == LOW)
      speedState = HIGH;
    else
      speedState = LOW;

    //  the LED with the ledState of the variable:
    digitalWrite(7, speedState);
  }
  
// Mike McRoberts serial input command routines
// from the "Serial Controlled Mood Lamp" example
// in Arduino Starter Kit Manual from Earthshine Design

if (Serial.available() > 0) {
  int index=0;
  delay(1); // let the buffer fill up
  int numChar = Serial.available();
  if (numChar>buffsize) {
      numChar=buffsize;
  }
  while (numChar--) {
      buffer[index++] = Serial.read();
  }
  splitString(buffer); // Process Serial Packet
  }
}

//----[ SubRoutines ]----------------------------------------------------

void splitString(char* data) {
// also from "Serial Controlled Mood Lamp" example

if (debug) {
 Serial.print("Data entered: ");
 Serial.println(data);
}

// Sequentially De-Tokenize the Serial Commands received
 char* parameter;
       parameter = strtok (data, " ,");
   while (parameter != NULL) {
          // Pass result to parseCMD for each Command received
          parseCMD(parameter);
          // remove processed commands from the list
          parameter = strtok (NULL, " ,");
   }

   // Clear the text and serial buffers
   for (int x=0; x<buffsize; x++) {
        buffer[x]='\0';
   }
  Serial.flush();
}

//=======================================================================
void parseCMD(char* data) {
// Flexible, easily expanded Command Parser
// based on "Serial Controlled Mood Lamp" example
// *** Marvelous coding by Mike MCRoberts

//--[gear]---------------------------------------
if ((data[0] == 'G') || (data[0] == 'g')) {
 // Have command, now get Argument "value" while removing whitespace
 int ArgVal = strtol(data+1, NULL, 10);
 // then limit the results to what we expect for this command
     ArgVal = constrain(ArgVal,0,8);
     //sevenSegWrite(ArgVal);
     

     if (debug) {
         Serial.print("Gear is set to: ");
         Serial.println(ArgVal);
     }
}

//--[speed]--------------------------------
if ((data[0] == 'S') || (data[0] == 's')) {
 int ArgVal = strtol(data+1, NULL, 10);
   
    //Serial.println(ArgVal);
       //shiftWrite(bargraph[ArgVal]);
       speedo = ArgVal/10;
       //if(speedo > 1000 && speedo < 11000){
          speedTime =(long)(10000.0*40/(speedo-1));//(gain*x+offs);
      // }
       // else if(speedo <= 1000){
      //    speedo = 1000;
      //  }
     //   else if(speedo >= 11000){
      //    speedo = 11000;
      //  }
     //   else{
      //    speedo = 0;
     //   }
        
     if (debug) {
         Serial.print("SPEED is : ");
         Serial.println(ArgVal);
     }
}

//--[reset]--------------------------------------
 if ((data[0] == 'C') || (data[0] == 'c')) {
   int ArgVal = strtol(data+1, NULL, 10);
       ArgVal = constrain(ArgVal,0,1);

       //sevenSegWrite(8);
       //shiftWrite(0x00);

     if (debug) {
         Serial.print("Clear Outputs");
     }
 }

 //--[rpm bar graph]-----------------------------
 if ((data[0] == 'R') || (data[0] == 'r')) {
   int ArgVal = strtol(data+1, NULL, 10);
   //int ArgVal2 = constrain(ArgVal,0,8);
    //Serial.println(ArgVal);
       //shiftWrite(bargraph[ArgVal]);
       rpm = ArgVal;
       if(rpm > 1000 && rpm < 11000){
          rpmTime =(long)(1000000.0*10.5/(rpm-325));//(gain*x+offs);
        }
        else if(rpm <= 1000){
          rpm = 1000;
        }
        else if(rpm >= 11000){
          rpm = 11000;
        }
        else{
          rpm = 0;
        }
        
     /*  
       
      if(ArgVal2 > 4){
         digitalWrite(2, HIGH);
       }
       else{
         digitalWrite(2, LOW);
       }
       if(ArgVal2 > 6){
         digitalWrite(3, HIGH);
       }
       else{
         digitalWrite(3, LOW);
       }
       if (debug) {
           Serial.print("RPM is set to: ");
           Serial.println(ArgVal2);
       }*/
 }
//--[rpm bar graph]-----------------------------
 if ((data[0] == 'W') || (data[0] == 'w')) {
   int ArgVal = strtol(data+1, NULL, 10);
   //int ArgVal2 = constrain(ArgVal,0,8);
    //Serial.println(ArgVal);
       //shiftWrite(bargraph[ArgVal]);
       switch(ArgVal){
         case 0: //Off
           analogWrite(9, 0); 
           break;
         case 1: //Cold
           analogWrite(9, 100); 
           break;
         case 2: //OK
           analogWrite(9, 200); 
           break;
         case 3: //Hot
           analogWrite(9, 230); 
           break;
         default:
           analogWrite(9, 0);                      
       } 
 }
    
//--[fuel]-----------------------------
 if ((data[0] == 'K') || (data[0] == 'k')) {
   int ArgVal = strtol(data+1, NULL, 10);
   //int ArgVal2 = constrain(ArgVal,0,8);
    //Serial.println(ArgVal);
       //shiftWrite(bargraph[ArgVal]);
       /*switch(ArgVal){
         case 0: //Off
           analogWrite(9, 0); 
           break;
         case 1: //Cold
           analogWrite(9, 100); 
           break;
         case 2: //OK
           analogWrite(9, 200); 
           break;
         case 3: //Hot
           analogWrite(9, 230); 
           break;
         default:
           analogWrite(9, 0);                      
       } 
       */
       
       
       analogWrite(10,map(ArgVal,0,100,255,195));
       
 }   
    
} // End parseCMD Loop


//=======================================================================
void sevenSegWrite(byte digit) {
  byte pin = GIstartpin;
  for (byte segCount = 0; segCount < 7; ++segCount) {
    //digitalWrite(pin, seven_seg_digits[digit][segCount]);
    ++pin;
  }
 }
//=======================================================================
void shiftWrite(byte Rdata){
    // prepare the register for data
    //digitalWrite(latchPin, LOW);
    // shift out the bits:
    //shiftOut(dataPin, clockPin, MSBFIRST, Rdata);
    //Set the latch pin high to enable the outputs
    //digitalWrite(latchPin, HIGH);
}
