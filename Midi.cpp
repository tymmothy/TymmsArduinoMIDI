/*  Midi.cpp: Code for MIDI processing library, Arduino version
 *
 *             (c) 2003-2008 Tymm Twillman <tymm@booyaka.com>
 *
 *  This file is part of Tymm's Arduino Midi Library.
 *
 *  Tymm's Arduino Midi Library is free software: you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser General Public 
 *  License as published by the Free Software Foundation, either version 2.1 
 *  of the License, or (at your option) any later version.
 *
 *  Tymm's Arduino Midi Library is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Tymm's Arduino Midi Library.  If not, see
 *  <http://www.gnu.org/licenses/>.
 */
 
#include "HardwareSerial.h"
#include "WConstants.h"
#include "Midi.h"


// This is used for tracking when we're processing a proprietary stream of data
//  The assigned value is arbitrary; just for internal use.
static const int MODE_PROPRIETARY = 0xff;


// These are midi status message types as sent on the wire
static const int STATUS_EVENT_NOTE_OFF        = 0x80;
static const int STATUS_EVENT_NOTE_ON         = 0x90;
static const int STATUS_EVENT_VELOCITY_CHANGE = 0xA0;
static const int STATUS_EVENT_CONTROL_CHANGE  = 0xB0;
static const int STATUS_EVENT_PROGRAM_CHANGE  = 0xC0;
static const int STATUS_AFTER_TOUCH           = 0xD0;
static const int STATUS_PITCH_CHANGE          = 0xE0;
static const int STATUS_START_PROPRIETARY     = 0xF0;
static const int STATUS_SONG_POSITION         = 0xF2;
static const int STATUS_SONG_SELECT           = 0xF3;
static const int STATUS_TUNE_REQUEST          = 0xF6;
static const int STATUS_END_PROPRIETARY       = 0xF7;
static const int STATUS_SYNC                  = 0xF8;
static const int STATUS_START                 = 0xFA;
static const int STATUS_CONTINUE              = 0xFB;
static const int STATUS_STOP                  = 0xFC;
static const int STATUS_ACTIVE_SENSE          = 0xFE;
static const int STATUS_RESET                 = 0xFF;
    


// Handle decoding incoming MIDI traffic a byte at a time -- remembers
//  what it needs to from one call to the next.
//
//  This is a private function & not meant to be called from outside this class.
//  It's used whenever data is available from the serial port.
//
void Midi::recvByte(int byte)
{
    int tmp;
    int channel;
    int bigval;           /*  temp 14-bit value for pitch, song pos */


    if (recvMode_ & MODE_PROPRIETARY
      && byte != STATUS_END_PROPRIETARY)
    {
        /* If proprietary handling compiled in, just pass all data received
         *  after a START_PROPRIETARY event to proprietary_decode
         *  until get an END_PROPRIETARY event
         */

#ifdef CONFIG_MIDI_PROPRIETARY
        proprietaryDecode(byte);
#endif

        return;
    }

    if (byte & 0x80) {
    
        /* All < 0xf0 events get at least 1 arg byte so
         *  it's ok to mask off the low 4 bits to figure
         *  out how to handle the event for < 0xf0 events.
         */

        tmp = byte;

        if (tmp < 0xf0)
            tmp &= 0xf0;

        switch (tmp) {
            /* These status events take 2 bytes as arguments */
            case STATUS_EVENT_NOTE_OFF:
            case STATUS_EVENT_NOTE_ON:
            case STATUS_EVENT_VELOCITY_CHANGE:
            case STATUS_EVENT_CONTROL_CHANGE:
            case STATUS_PITCH_CHANGE:
            case STATUS_SONG_POSITION:
                recvBytesNeeded_ = 2;
                recvByteCount_ = 0;
                recvEvent_ = byte;
                break;

            /* 1 byte arguments */
            case STATUS_EVENT_PROGRAM_CHANGE:
            case STATUS_AFTER_TOUCH:
            case STATUS_SONG_SELECT:
                recvBytesNeeded_ = 1;
                recvByteCount_ = 0;
                recvEvent_ = byte;
                return;

            /* No arguments ( > 0xf0 events) */
            case STATUS_START_PROPRIETARY:
                recvMode_ |= MODE_PROPRIETARY;

#ifdef CONFIG_MIDI_PROPRIETARY
                proprietaryDecodeStart();
#endif

                break;
            case STATUS_END_PROPRIETARY:
                recvMode_ &= ~MODE_PROPRIETARY;

#ifdef CONFIG_MIDI_PROPRIETARY
                proprietaryDecodeEnd();
#endif

                break;
            case STATUS_TUNE_REQUEST:
                handleTuneRequest();
                break;
            case STATUS_SYNC:
                handleSync();
                break;
            case STATUS_START:
                handleStart();
                break;
            case STATUS_CONTINUE:
                handleContinue();
                break;
            case STATUS_STOP:
                handleStop();
                break;
            case STATUS_ACTIVE_SENSE:
                handleActiveSense();
                break;
            case STATUS_RESET:
                handleReset();
                break;
        }

        return;
    }

    if (++recvByteCount_ == recvBytesNeeded_) {
        /* Copy out the channel (if applicable; in some cases this will be meaningless,
         *  but in those cases the value will be ignored)
         */
        channel = (recvEvent_ & 0x0f) + 1;

        tmp = recvEvent_;
        if (tmp < 0xf0) {
            tmp &= 0xf0;
        }

        /* See if this event matches our MIDI channel
         *  (or we're accepting for all channels)
         */
        if (!channelIn_
             || (channel == channelIn_)
             || (tmp >= 0xf0))
        {
            switch (tmp) {
                case STATUS_EVENT_NOTE_ON:
                    /* If velocity is 0, it's actually a note off & should fall thru
                     *  to the note off case
                     */
                    if (byte) {
                        handleNoteOn(channel, recvArg0_, byte);
                        break;
                    }

                case STATUS_EVENT_NOTE_OFF:
                    handleNoteOff(channel, recvArg0_, byte);
                    break;
                case STATUS_EVENT_VELOCITY_CHANGE:
                    handleVelocityChange(channel, recvArg0_, byte);
                    break;
                case STATUS_EVENT_CONTROL_CHANGE:
                    handleControlChange(channel, recvArg0_, byte);
                    break;
                case STATUS_EVENT_PROGRAM_CHANGE:
                    handleProgramChange(channel, byte);
                    break;
                case STATUS_AFTER_TOUCH:
                    handleAfterTouch(channel, byte);
                    break;
                case STATUS_PITCH_CHANGE:
                    bigval = (byte << 7) | recvArg0_;
                    handlePitchChange(bigval);
                    break;
                case STATUS_SONG_POSITION:
                    bigval = (byte << 7) | recvArg0_;
                    handleSongPosition(bigval);
                    break;
                case STATUS_SONG_SELECT:
                    handleSongSelect(byte);
                    break;
            }
        }

        /* Just reset the byte count; keep the same event -- might get more messages
            trailing from current event.
         */
        recvByteCount_ = 0;
    }
    
    recvArg0_ = byte;
}


// Try to read data at serial port & pass anything read to processing function
void Midi::poll(void)
{
    int c;
    
    
    // Just keep sucking data from serial port until it runs out, processing
    //  MIDI messages as we go
    while((c = serial_.read()) != -1) {
        recvByte(c);
    }
}


// Send Midi NOTE OFF message to a given channel, with note 0-127 and velocity 0-127
void Midi::sendNoteOff(unsigned int channel, unsigned int note, unsigned int velocity)
{
    int status = STATUS_EVENT_NOTE_OFF | ((channel - 1) & 0x0f);
    
    
    if (sendFullCommands_ || (lastStatusSent_ != status)) {
        serial_.print(status, BYTE);
    }
    
    serial_.print(note & 0x7f, BYTE);
    serial_.print(velocity & 0x7f, BYTE);
}


// Send Midi NOTE ON message to a given channel, with note 0-127 and velocity 0-127
void Midi::sendNoteOn(unsigned int channel, unsigned int note, unsigned int velocity)
{
    int status = STATUS_EVENT_NOTE_ON | ((channel - 1) & 0x0f);
    
    
    if (sendFullCommands_ || (lastStatusSent_ != status)) {
        serial_.print(status, BYTE);
    }
    
    serial_.print(note & 0x7f, BYTE);
    serial_.print(velocity & 0x7f, BYTE);
}


// Send a Midi VELOCITY CHANGE message to a given channel, with given note 0-127,
//  and new velocity 0-127
void Midi::sendVelocityChange(unsigned int channel, unsigned int note, unsigned int velocity)
{
    int status = STATUS_EVENT_VELOCITY_CHANGE | ((channel - 1) & 0x0f);
    
    
    if (sendFullCommands_ || (lastStatusSent_ != status)) {
        serial_.print(status, BYTE);
    }
    
    serial_.print(note & 0x7f, BYTE);
    serial_.print(velocity & 0x7f, BYTE);
}


// Send a Midi CC message to a given channel, as a given controller 0-127, with given
//  value 0-127
void Midi::sendControlChange(unsigned int channel, unsigned int controller, unsigned int value)
{
    int status = STATUS_EVENT_CONTROL_CHANGE | ((channel - 1) & 0x0f);
    
    
    if (sendFullCommands_ || (lastStatusSent_ != status)) {
        serial_.print(status, BYTE);
    }
    
    serial_.print(controller & 0x7f, BYTE);
    serial_.print(value & 0x7f, BYTE);
}


// Send a Midi PROGRAM CHANGE message to given channel, with program ID 0-127
void Midi::sendProgramChange(unsigned int channel, unsigned int program)
{
    int status = STATUS_EVENT_PROGRAM_CHANGE | ((channel - 1) & 0x0f);
    
    
    if (sendFullCommands_ || (lastStatusSent_ != status)) {
        serial_.print(status, BYTE);
    }

    serial_.print(program & 0x7f, BYTE);
}


// Send a Midi AFTER TOUCH message to given channel, with velocity 0-127
void Midi::sendAfterTouch(unsigned int channel, unsigned int velocity)
{
    int status = STATUS_AFTER_TOUCH | ((channel - 1) & 0x0f);
    
    
    if (sendFullCommands_ || (lastStatusSent_ != status)) {
        serial_.print(status, BYTE);
    }

    serial_.print(velocity & 0x7f, BYTE);
}


// Send a Midi PITCH CHANGE message, with a 14-bit pitch (always for all channels)
void Midi::sendPitchChange(unsigned int pitch)
{
    serial_.print(STATUS_PITCH_CHANGE, BYTE);
    serial_.print(pitch & 0x7f);
    serial_.print((pitch >> 7) & 0x7f);
}


// Send a Midi SONG POSITION message, with a 14-bit position (always for all channels)
void Midi::sendSongPosition(unsigned int position)
{
    serial_.print(STATUS_SONG_POSITION, BYTE);
    serial_.print(position & 0x7f);
    serial_.print((position >> 7) & 0x7f);
}


// Send a Midi SONG SELECT message, with a song ID of 0-127 (always for all channels)
void Midi::sendSongSelect(unsigned int song)
{
    serial_.print(STATUS_SONG_SELECT, BYTE);
    serial_.print(song & 0x7f, BYTE);
}


// Send a Midi TUNE REQUEST message (TUNE REQUEST is always for all channels)
void Midi::sendTuneRequest(void)
{
    serial_.print(STATUS_TUNE_REQUEST, BYTE);
}


// Send a Midi SYNC message (SYNC is always for all channels)
void Midi::sendSync(void)
{
    serial_.print(STATUS_SYNC, BYTE);
}


// Send a Midi START message (START is always for all channels)
void Midi::sendStart(void)
{
    serial_.print(STATUS_START, BYTE);
}


// Send a Midi CONTINUE message (CONTINUE is always for all channels)
void Midi::sendContinue(void)
{
    serial_.print(STATUS_CONTINUE, BYTE);
}


// Send a Midi STOP message (STOP is always for all channels)
void Midi::sendStop(void)
{
    serial_.print(STATUS_STOP, BYTE);
}


// Send a Midi ACTIVE SENSE message (ACTIVE SENSE is always for all channels)
void Midi::sendActiveSense(void)
{
    serial_.print(STATUS_ACTIVE_SENSE, BYTE);
}


// Send a Midi RESET message (RESET is always for all channels)
void Midi::sendReset(void)
{
    serial_.print(STATUS_RESET, BYTE);
}


// Constructor -- set up defaults for variables, get ready for use (but don't
//  take over serial port yet)
Midi::Midi(HardwareSerial &serial)
{
    /* Not in proprietary stream */
    recvMode_ = 0;
    /* No bytes recevied */
    recvByteCount_ = 0;
    /* Not processing an event */
    recvEvent_ = 0;
    /* No arguments to the event we haven't received */
    recvArg0_ = 0;
    /* Not waiting for bytes to complete a message */
    recvBytesNeeded_ = 0;
    // There was no last event.
    lastStatusSent_ = false;
    // Don't send the extra bytes; just send deltas
    sendFullCommands_ = false;

    /* Listening to all channels */
    channelIn_ = 0;
    
    serial_ = serial;
}


// Open the serial port and begin processing.
void Midi::begin(unsigned int channel, unsigned long baud)
{
  channelIn_ = channel; 
  serial_.begin(baud);
}


// Set (package-specific) parameters for the Midi instance
void Midi::setParam(unsigned int param, unsigned int val)
{
    if (param == PARAM_SEND_FULL_COMMANDS) {
        if (val) {
            sendFullCommands_ = true;
        } else {
            sendFullCommands_ = false;
        }
    } else if (param == PARAM_CHANNEL_IN) {
        channelIn_ = val;
    }
}


// Get (package-specific) parameters for the Midi instance
unsigned int Midi::getParam(unsigned int param)
{
    if (param == PARAM_SEND_FULL_COMMANDS) {
        return sendFullCommands_;
    } else if (param == PARAM_CHANNEL_IN) {
        return channelIn_;
    }
    
    return 0;
}


// Placeholders.  You should subclass the Midi base class and define these to have your 
//  version called.
void Midi::handleNoteOff(unsigned int channel, unsigned int note, unsigned int velocity) {}
void Midi::handleNoteOn(unsigned int channel, unsigned int note, unsigned int velocity) {}
void Midi::handleVelocityChange(unsigned int channel, unsigned int note, unsigned int velocity) {}
void Midi::handleControlChange(unsigned int channel, unsigned int controller, unsigned int value) {}
void Midi::handleProgramChange(unsigned int channel, unsigned int program) {}
void Midi::handleAfterTouch(unsigned int channel, unsigned int velocity) {}
void Midi::handlePitchChange(unsigned int pitch) {}
void Midi::handleSongPosition(unsigned int position) {}
void Midi::handleSongSelect(unsigned int song) {}
void Midi::handleTuneRequest(void) {}
void Midi::handleSync(void) {}
void Midi::handleStart(void) {}
void Midi::handleContinue(void) {}
void Midi::handleStop(void) {}
void Midi::handleActiveSense(void) {}
void Midi::handleReset(void) {}
