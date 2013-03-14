///////////////////////////////////////////////////////////////////
 /*  FabFm Firmware v11 
 *   
 *   Aaron Weiss, SparkFun 2012
 *   license: beerware
 *   
 *   Description:
 *   This is example code for the FamFM radio kit. The hardware
 *   mimics an Arduino Pro (3.3V, 8MHz) and has an FTDI header 
 *   to load Arduino sketches. The hardware includes the Si4703
 *   FM radio module to find the stations along with an op-amp 
 *   and speaker for the audio. The unit has a potentiometer
 *   with a switch for power and volume control, and a rotary
 *   encoder to tune the stations. The unit displays the radio
 *   station over a serial terminal at 115200 and also saves the 
 *   station on subsequent power ups. 
 *
 *   The Si4703 libarary needs to be used:
 *   http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/Si4703_Breakout-Arduino_1_compatible.zip
 *   
 *   Operation:
 *   -Power switch and volume are on the left, tuner is on the right.
 *   -The board must be powered with a switch mode 9V DC wall wart.
 *   -Each click will increase/decrease station by 0.2MHz 
 *   -The LED blinks for every step
 */   
///////////////////////////////////////////////////////////////////

// Libraries needed
// Si4703 lib: See above
#include <Si4703_Breakout.h>
// included with Arduino
#include <Wire.h>
// included with Arduino
#include <EEPROM.h>

// Global Constants (defines): these quantities don't change
const int resetPin = 4; //radio reset pin
const int SDIO = A4; //radio data pin
const int SCLK = A5; //radio clock pin
const int encoderPin1 = 2; // encoder pin 1
const int encoderPin2 = 3; // encoder pin 2
const int LED = 5; //optional LED pin

// Global Variables: these quantities change
int channel;
int rotate;

//Volatile variables are needed if used within interrupts
volatile int lastEncoded = 0;
volatile long encoderValue = 0;

volatile int goodEncoderValue;
volatile boolean updateStation = false;

const boolean UP = true;
const boolean DOWN = false;
volatile boolean stationDirection;

// You must call this to define the pins and enable the FM radio 
// module.
Si4703_Breakout radio(resetPin, SDIO, SCLK);

// Arduino setup function
void setup()
{
  // both pins on the rotary encoder are inputs and pulled high
  pinMode(encoderPin1, INPUT_PULLUP);
  pinMode(encoderPin2, INPUT_PULLUP);
  
  // LED pin is output
  pinMode(LED, OUTPUT);
  
  // load saved station into channel variable
  read_channel_from_EEPROM();
  
  // start radio module
  radio.powerOn(); // turns the module on
  radio.setChannel(channel); // loads saved channel
  radio.setVolume(15); //Loudest volume setting
  digitalWrite(LED, HIGH); //turn LED ON
  
  //start serial and print welcome screen with station number
  Serial.begin(115200); //must use a fast baud to prevent errors
  Serial.print("\nfab fm: ");
  Serial.println(channel, DEC);

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3) 
  attachInterrupt(0, updateEncoder, CHANGE); 
  attachInterrupt(1, updateEncoder, CHANGE);
}

// Arduino main loop
void loop()
{
  //Wait until the interrupt tells us to update the station
  if(updateStation)
  {
    digitalWrite(LED, LOW);

    if(stationDirection == UP)
    {
      //Serial.print("Up ");
      channel += 2; //Channels change by 2 (975 to 973)
    }
    else if(stationDirection == DOWN)
    {
      //Serial.print("Down ");
      channel -= 2; //Channels change by 2 (975 to 973)
    }
    
    //Catch wrap conditions
    if(channel > 1079) channel = 875;
    if(channel < 875) channel = 1079;

    Serial.println(channel, DEC); // print channel number

    radio.setChannel(channel); //Goto the new channel
    save_channel(); // save channel to EEPROM

    digitalWrite(LED, HIGH);
    
    updateStation = false; //Clear flag
  }

  // You can put any additional code here, but keep in mind, 
  // the encoder interrupt is running in the background
}

// NAME: save_channel() 
// DEFINITION: splits the channel variable into two separate bytes, 
// then loads each value into one 8-bit EEPROM location
void save_channel()
{
  int msb = channel >> 8; // move channel over 8 spots to grab MSB
  int lsb = channel & 0x00FF; // clear the MSB, leaving only the LSB
  EEPROM.write(1, msb); // write each byte to a single 8-bit position
  EEPROM.write(2, lsb);
}

// NAME: read_channel_from_EEPROM() 
// DEFINITION: reads the two saved 8-bit values, that defines the 
// channel, and concatenates them together to form a 16-bit number 
void read_channel_from_EEPROM()
{
  int msb = EEPROM.read(1); // load the msb into one 8-bit register
  int lsb = EEPROM.read(2); // load the lsb into one 8-bit register
  msb = msb << 8; // shift the msb over 8 bits
  channel = msb|lsb; // concatenate the lsb and msb
}

//This is the interrupt that reads the encoder
//It set the updateStation flag when a new indent is found 
//Note: The encoder used on the FabFM has a mechanical indent every four counts
//So most of the time, when you advance 1 click there are four interrupts
//But there is no guarantee the user will land neaty on an indent, so this code cleans up the read
//Modified from the bildr article: http://bildr.org/2012/08/rotary-encoder-arduino/
void updateEncoder()
{
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
  {
    stationDirection = DOWN; //Counter clock wise
    encoderValue--;
  }
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
  {
    stationDirection = UP; //Clock wise
    encoderValue++;
  }

  lastEncoded = encoded; //store this value for next time

  //Wait until we are more than 3 ticks from previous used value
  if(abs(goodEncoderValue - encoderValue) > 3)
  {
    //The user can sometimes miss an indent by a count or two
    //This logic tries to correct for that
    //Remember, interrupts are happening in the background so encoderValue can change 
    //throughout this code

    if(encoderValue % 4 == 0) //Then we are on a good indent
    {
      goodEncoderValue = encoderValue; //Remember this good setting
    }
    else if( abs(goodEncoderValue - encoderValue) == 3) //The encoder is one short
    {
      if(encoderValue < 0) goodEncoderValue = encoderValue - 1; //Remember this good setting
      if(encoderValue > 0) goodEncoderValue = encoderValue + 1; //Remember this good setting
    }
    else if( abs(goodEncoderValue - encoderValue) == 5) //The encoder is one too long
    {
      if(encoderValue < 0) goodEncoderValue = encoderValue + 1; //Remember this good setting
      if(encoderValue > 0) goodEncoderValue = encoderValue - 1; //Remember this good setting
    }

    updateStation = true;
  }
}

