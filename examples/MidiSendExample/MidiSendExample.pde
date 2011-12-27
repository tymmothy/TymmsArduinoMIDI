// This sketch is for building a simple MIDI controller with 2 buttons for
//  sending note values & 2 knobs for control values
//
// To get this working, you should have set up the following hardware:
//  - a switch from Digital Pin 2 to +5v, and a 1 to 10K resistor from 
//      pin 2 to ground
//  - a switch from Digital Pin 3 to +5v, and a 1 to 10K resistor from
//      pin 3 to ground
//  - a potentiometer with one side at +5v, one side at ground, and the
//      wiper (center pin) connected to Analog Pin 0
//  - a potentiometer with one side at +5v, one side at ground, and the
//      wiper (center pin) connected to Analog Pin 1
//

#include "Midi.h"

// Create an instance of the Midi class.
//
// If we were had to receive messages, you'd have to make a subclass of the
//  Midi class.  See the MidiReceiveExample for an example of how to do that.
//
// Sending is a lot easier :)
Midi midi(Serial);


void setup()
{
  // Configure digital pins for input.
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  
  // This causes the midi object to listen to ALL Midi channels.  If a number
  //  from 1-16 is passed, messages will be filtered so that any messages to
  //  channels other than the given one will be ignored.
  midi.begin(0);
}

int lastDigital2 = LOW;
int lastDigital3 = LOW;

int lastAnalog0 = 0;
int lastAnalog1 = 0;

// MIDI channel to send messages to
int channelOut = 1;


void doControls()
{
  int a;


  // Changes on analog lines cause control change events
  //  to be sent; these are parameters from 0-127
  
  a = analogRead(0) / 8 ;
  if (a != lastAnalog0) {
    midi.sendControlChange(channelOut, 15, a);
    lastAnalog0 = a;
  }

  a = analogRead(1) / 8;
  if (a != lastAnalog1) {
    midi.sendControlChange(channelOut, 16, a);
    lastAnalog1 = a;
  }
}


void doKeys()
{
  int d;
  
  
  // Basic code to check pins 2-5; if there's a change on the pin, send
  //  a NOTE ON or NOTE OFF, depending on whether the button was pressed
  //  or released, with a different note value for each button.  The velocity
  //  for all of the NOTE ON messages is the max (127)

  d = digitalRead(2);
  if (d != lastDigital2) {
    if (d == HIGH) {
      midi.sendNoteOn(channelOut, 40, 127);
    } else {
      midi.sendNoteOff(channelOut, 40, 0);
    }
    lastDigital2 = d;
  }
  
  d = digitalRead(3);
  if (d != lastDigital3) {
    if (d == HIGH) {
      midi.sendNoteOn(channelOut, 41, 127);
    } else {
      midi.sendNoteOff(channelOut, 41, 0);
    }
    lastDigital3 = d;
  }
}


void loop()
{
  // DON'T need to call midi.poll(), because this sketch doesn't need to
  //  receive any Midi data.
  doKeys();
  doControls();
}
