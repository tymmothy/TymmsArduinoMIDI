/*
 * This sketch was for a project controlling pneumatics via Midi -- I used 4
 *  daisychained TPIC6B595 shift registers to control 32 pneumatic solenoids
 *  requiring 24v control signals.  In my setup I used Garage Band as the
 *  sequencer (since it was already on my machine), with midiO plugin,
 *  sending Midi to Max/MSP, where I connected a Midi In object to a serial
 *  object that sent the data to my Arduino.
 *
 * A little complicated, but it worked reasonably well.
 */

#include "Midi.h"


// Pins used to communicate with the TPIC6B595 shiftg registers
#define D_PIN     2 
#define RCK_PIN   3
#define SRCK_PIN  4
#define SRCLR_PIN 5
#define EN_PIN    6

// Number of shift registers in the chain
#define NUM_TPIC6B595 4


// Subclassing Midi class to override the note handling functions
class MyMidi : public Midi {
  public:
  
  MyMidi(HardwareSerial &s) : Midi(s) {}
  
  void handleNoteOn(unsigned int channel, unsigned int note, unsigned int velocity)
  {
    // indicator bit to show note on received
    digitalWrite(13, HIGH);
    
    // set the pin corresponding to the note high (open the valve); the 
    //  note & 31 bit is to limit the pin # from 0-31 since otherwise
    //  with a high note we could end up writing memory that's outside
    //  the array
    tpic6b595_set((note & 31), HIGH);
    tpic6b595_load();
  }

  void handleNoteOff(unsigned int channel, unsigned int note, unsigned int velocity)
  {
    // This is pretty much the same as handleNoteOn, except it turns things off
    //  instead of on.
    
    digitalWrite(13, LOW);
    tpic6b595_set((note & 31), LOW);
    tpic6b595_load();
  }
};


// Since the shift registers are read-only, need to keep a copy of the current
//  state of each output locally
unsigned char tpic6b595_vals[NUM_TPIC6B595];

// Create the actual instance of our Midi class; attach it to the standard
//  serial port.
MyMidi midi(Serial);


// Function for initializing the pins for communicating with the TPIC6B595's
void tpic6b595_init()
{
  
  pinMode(D_PIN, OUTPUT);
  pinMode(RCK_PIN, OUTPUT);
  pinMode(SRCK_PIN, OUTPUT);
  pinMode(SRCLR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  
  digitalWrite(RCK_PIN, LOW);
  digitalWrite(SRCK_PIN, LOW);
  digitalWrite(SRCLR_PIN, LOW);
  digitalWrite(SRCLR_PIN, HIGH);
  digitalWrite(EN_PIN, LOW);
}


// Load the current state of the TPIC6B595 values into the shift registers &
//  latch the new values
void tpic6b595_load()
{
  for (int i = 0; i < NUM_TPIC6B595; i++) {
   shiftOut(D_PIN, SRCK_PIN, MSBFIRST, tpic6b595_vals[(NUM_TPIC6B595 - 1) - i]);
  }
  
  digitalWrite(RCK_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(RCK_PIN, LOW);
}


// Get the state of any of the TPIC6B595 pins (first chip is considered to 
//  be pins 0-7, chip 2 pins 8-15, etc.
unsigned char tpic6b595_get(unsigned char pin)
{
  return (tpic6b595_vals[pin / 8] & _BV(pin & 7));
}


// Set the state of any of the TPIC6B595 pins (again, first chip is pins 0-7,
//  chip 2 pins 8-15, etc.
void tpic6b595_set(unsigned char pin, unsigned char val)
{
  if (val) {
    tpic6b595_vals[pin / 8] |= _BV(pin & 7);
  } else {
    tpic6b595_vals[pin / 8] &= ~_BV(pin & 7);
  }
}


// Set pin 13 to output for some indication of data being received, initialize the
//  shift registers, start the Midi instance and turn off all shift register outputs
void setup()
{
  pinMode(13, OUTPUT);
  tpic6b595_init();
  midi.begin(0, 115200);
  
  tpic6b595_load();
}


// All we gotta do is process Midi data... 
void loop()
{
  midi.poll();
}
