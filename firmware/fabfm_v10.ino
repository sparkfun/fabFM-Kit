///////////////////////////////////////////////////////////////////
 /*  FabFm Firmware v10 
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
 *   The Si7703 libarary needs to be used:
 *   http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/Si4703_Breakout-Arduino_1_compatible.zip
 *   
 *   Operation:
 *   -Power switch and volume are on the left, tuner is on the right.
 *   -The board must be powered with a switch mode 9V DC wall wart.
 *   -To tune the station, the encoder must be turned at least 3clicks. 
 *   -The LED turns on when you are on a station.
 *   -The tuning moves to stations with a low RSSI (recieved signal
 *    strength indicator) so some numbers will be skipped. 
 */   
///////////////////////////////////////////////////////////////////

// Libraries needed
// Si4703 lib: http://dl.dropbox.com/u/3993179/Si4703_Breakout.zip
#include <Si4703_Breakout.h>
// included with Arduino
#include <Wire.h>
// included with Arduino
#include <EEPROM.h>

// Global Constants (defines): these quantities don't change
const int resetPin = 4; //radio reset pin
const int SDIO = A4; //radio data pin
const int SCLK = A5; //radio clock pin
const int E1 = 2; // encoder pin 1
const int E2 = 3; // encoder pin 2
const int LED = 5; //optional LED pin

// Global Variables: these quantities change
int channel;
int rotate;

// You must call this to define the pins and enable the FM radio 
// module.
Si4703_Breakout radio(resetPin, SDIO, SCLK);

// Arduino setup function
void setup()
{
  // both pins on the rotary encoder are inputs and pulled high
  pinMode(E1, INPUT);
  digitalWrite(E1, HIGH);
  pinMode(E2, INPUT);
  digitalWrite(E2, HIGH);
  
  // LED pin is output
  pinMode(LED, OUTPUT);
  
  // load saved station into channel variable
  read_channel_from_EEPROM();
  
  // start radio module
  radio.powerOn(); // turns the module on
  radio.setChannel(channel); // loads saved channel
  digitalWrite(LED, HIGH); //turn LED ON
  
  //start serial and print welcome screen with station number
  Serial.begin(115200); //must use a fast baud to prevent errors
  Serial.print("\nfab fm: ");
  Serial.println(channel, DEC);
}

// Arduino main loop
void loop()
{
  seek_encoder(); //used to find a station based on the encoder
  
  // You can put any additional code here, but keep in mind, 
  // the more time spent out of seek_encoder, the easier it will
  // be to skip over a station or cause seek errors.
}

// NAME: read_encoder(), from circuitsathome.com
// DEFINITION: returns a 0 if there is no rotation on the encoder,
// returns a -1 for clockwiese and 1 for counter-clockwise
int read_encoder()
{
  // more information on this function can be found here:
  // http://www.circuitsathome.com/mcu/reading-rotary-encoder-on-arduino
  // enc_states[] array is a lookup table for possible encoder 
  // states, defined as static to retain value between function 
  // calls
  static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  static unsigned int old_AB = 0; //temporary variable

  old_AB >>= 2; // remember previous state
  old_AB |= (PIND & 0x0C);  // add current state, set port pins 2 
                            // and 3 in PIND
  // the final bitwise operation results in the previous state in 
  // bit positions 2 and 3 and the current state in 0 and 1. The
  // resulting value "points" to the correct array element.
  return ( enc_states[( old_AB & 0x0f )]);
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

// NAME: seek_encoder() 
// DEFINITION: reads read_encoder(), then adjusts and prints the 
// station number
void seek_encoder()
{
  // Create a variable that will increment or decrement based on
  // the direction of the rotation.
  static uint8_t counter = 125;
  
  // Read the encoder, rotate can be either 1 (CCW rotation), 
  // -1 (CW rotation), or 0 (no rotation).
  rotate = read_encoder();
  
  // If the encoder is rotated, add the encoder state to counter.
  if(rotate)
  {  
    counter += rotate;
    //digitalWrite(LED, LOW);
  }
  
  // if clockwise rotation
  if(counter < 115)
  {  
    digitalWrite(LED, LOW);
    channel = radio.seekUp(); // seek down and save channel value
    Serial.println(channel, DEC); // print channel number
    save_channel(); // save channel to EEPROM
    counter = 125;
    digitalWrite(LED, HIGH);
  }

  // if counter clockwise rotation
  if(counter > 135)
  {
    digitalWrite(LED, LOW);
    channel = radio.seekDown(); // seek up and save channel value
    Serial.println(channel, DEC); // print channel number
    save_channel(); // save channel to EEPROM
    counter = 125;
    digitalWrite(LED, HIGH);
  }
}
