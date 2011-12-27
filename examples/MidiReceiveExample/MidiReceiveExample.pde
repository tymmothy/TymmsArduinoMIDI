// This sketch turns on the LED at D13 when any "NOTE ON" message is received.
//  It turns the LED off when any "NOTE OFF" message is received.
//
// (note that NOTE ON's with velocity = 0 are actually NOTE OFF's, and are
//  treated as such)
//
// It's necessary to subclass Midi in order to handle receiving messages.  If
//  you're only sending data, see MidiSendExample, which is a bit simpler.
//
// If you're both sending AND receiving you'll need to subclass like this,
//  but only for receive functions.
//

#include "Midi.h"

// To use the Midi class for receiving Midi messages, you need to subclass it.
//  If you're a C++ person this makes sense; if not, just follow the example
//  functions below.
//
// Basically, MyMidi here can do anything Midi can do -- it just has the ability
//  to easily change what the functions do when different message types are received.
//
class MyMidi : public Midi {
  public:
  
  // Need this to compile; it just hands things off to the Midi class.
  MyMidi(HardwareSerial &s) : Midi(s) {}
  
  void handleNoteOn(unsigned int channel, unsigned int note, unsigned int velocity)
  {
    digitalWrite(13, HIGH);
  }

  void handleNoteOff(unsigned int channel, unsigned int note, unsigned int velocity)
  {
    digitalWrite(13, LOW);
  }
  
  /* You can define any of these functions and they will be called when the
     matching message type is received.  Otherwise those types of Midi messages
     are just ignored.

    For C++ folks: these are all declared virtual in the base class

    void handleNoteOff(unsigned int channel, unsigned int note, unsigned int velocity);
    void handleNoteOn(unsigned int channel, unsigned int note, unsigned int velocity);
    void handleVelocityChange(unsigned int channel, unsigned int note, unsigned int velocity);
    void handleControlChange(unsigned int channel, unsigned int controller, unsigned int value);
    void handleProgramChange(unsigned int channel, unsigned int program);
    void handleAfterTouch(unsigned int channel, unsigned int velocity);
    void handlePitchChange(unsigned int pitch);
    void handleSongPosition(unsigned int position);
    void handleSongSelect(unsigned int song);
    void handleRuneRequest(void);
    void handleSync(void);
    void handleStart(void);
    void handleContinue(void);
    void handleStop(void);
    void handleActiveSense(void);
    void handleReset(void);
*/
};

// Create an instance of the MyMidi class.
MyMidi midi(Serial);


void setup()
{
  pinMode(13, OUTPUT);
  
  // This causes the midi object to listen to ALL Midi channels.  If a number
  //  from 1-16 is passed, messages will be filtered so that any messages to
  //  channels other than the given one will be ignored.
  //
  // Note that you can pass a second parameter as a baud rate, if you're doing
  //  Midi protocol but using some other communication method (e.g. sending
  //  Midi data from Max/MSP using the standard Arduino USB connection, you
  //  can set the baud rate to something more standard and use the regular
  //  Max serial object)
  midi.begin(0);
}


void loop()
{
  // Must be called every time through loop() IF dealing with incoming MIDI --
  //  if you're just sending MIDI out, you don't need to use poll().
  //
  // Make sure other code doesn't take too long to process or serial data
  //  can get backed up (and you can end up with obnoxious latency that ears
  //  don't like).
  //
  // The poll function will cause the midi code to suck data from the serial 
  //  port and process it, and call any functions that are defined for handling
  //  midi messages.
  midi.poll();
}
