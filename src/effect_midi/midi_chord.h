/* 
 * Chord generator
 * 
 * Javier Nonis (https://github.com/jnonis) - 2024
 */
#ifndef _MIDI_CHORD_H
#define _MIDI_CHORD_H

#include "midi_effect_base.h"
#include "modarpeggiator/common/commons.h"

class MidiChord : public MidiEffect
{
public:
public:
    // ID must be unique for each MidiEffect
    static const unsigned ID = 2;
    static constexpr const char* NAME = "Chord";

    enum Param
    {
        BYPASS,
        MODE,
        UNKNOWN
    };

    enum Mode
    {
        FIFTH,
        FIFTH_OCT_DOWN,
        FIFTH_OCT_UP,
        OCT_DOWN,
        OCT_UP,
        OCT_UP_DOWN,
        MAJOR,
        MINOR,
        MODE_UNKNOWN
    };

    MidiChord(float32_t samplerate, CDexedAdapter* synth) : MidiEffect(samplerate, synth)
    {
        mode = Mode::FIFTH;
        currentMode = Mode::FIFTH;
    }

    virtual ~MidiChord()
    {
        this->synth->panic();
        events.clear();
	    events.shrink_to_fit();
        pressed.clear();
	    pressed.shrink_to_fit();
    }

    virtual unsigned getId()
    {
        return MidiChord::ID;
    }

    virtual std::string getName()
    {
        return MidiChord::NAME;
    }

    virtual void setParameter(unsigned param, unsigned value)
    {
        switch (param)
        {
        case MidiChord::Param::BYPASS:
            this->setBypass(value == 1);
            break;
        case MidiChord::Param::MODE:
            this->mode = value;
            break;
        default:
            break;
        }
    }

    virtual unsigned getParameter(unsigned param)
    {
        switch (param)
        {
        case MidiChord::Param::BYPASS:
            this->synth->panic();
            return this->getBypass() ? 1 : 0;
        case MidiChord::Param::MODE:
            return this->mode;
        default:
            return 0;
        }
    }

    virtual void keydown(int16_t pitch, uint8_t velocity)
    {
        MidiEvent event;
        event.data[0] = MIDI_NOTE_ON << 4;
        event.data[1] = pitch;
        event.data[2] = velocity;
        event.size = 3;
        event.frame = 0;
        this->events.push_back(event);
    }

    virtual void keyup(int16_t pitch)
    {
        MidiEvent event;
        event.data[0] = MIDI_NOTE_OFF << 4;
        event.data[1] = pitch;
        event.data[2] = 0;
        event.size = 3;
        event.frame = 0;
        this->events.push_back(event);
    }

protected:
    virtual size_t getParametersSize()
    {
        return MidiChord::Param::UNKNOWN;
    }

    virtual void doProcess(uint16_t len)
    {
        // If mode changed
        if (currentMode != mode)
        {
            this->synth->panic();
            currentMode = mode;
            for (size_t i = 0; i < pressed.size(); i++)
            {
                MidiEvent event = pressed.data()[i];
                processNoteOn(event);
            }
        }

        // Check events
        for (size_t i = 0; i < events.size(); i++)
        {
            MidiEvent event = events.data()[i];
            if (event.data[0] >> 4 == MIDI_NOTE_ON)
            {
                pressed.push_back(event);
                processNoteOn(event);
            }
            else if (event.data[0] >> 4 == MIDI_NOTE_OFF)
            {
                remove(event);
                processNoteOff(event);
            }
        }
        events.clear();
	    events.shrink_to_fit();
    }

private:
    static const unsigned MIDI_NOTE_OFF = 0b1000;
    static const unsigned MIDI_NOTE_ON = 0b1001;

    unsigned mode;
    unsigned currentMode;
    std::vector<MidiEvent> events;
    std::vector<MidiEvent> pressed;

    void processNoteOn(MidiEvent event)
    {
        std::vector<MidiEvent> chord = getChord(event);
        for (size_t j = 0; j < chord.size(); j++)
        {
            MidiEvent chordNote = chord.data()[j];
            this->synth->keydown(chordNote.data[1], chordNote.data[2]);
        }
        chord.clear();
        chord.shrink_to_fit();
    }

    void processNoteOff(MidiEvent event)
    {
        std::vector<MidiEvent> chord = getChord(event);
        for (size_t j = 0; j < chord.size(); j++)
        {
            MidiEvent chordNote = chord.data()[j];
            this->synth->keyup(chordNote.data[1]);
        }
        chord.clear();
        chord.shrink_to_fit();
    }

    void remove(MidiEvent note)
    {
        for (unsigned i = 0; i < pressed.size(); i++)
        {
            if (pressed.data()[i].data[1] == note.data[1])
            {
                pressed.erase(pressed.begin() + i);
                break;
            }
        }
    }

    std::vector<MidiEvent> getChord(MidiEvent note)
    {
        std::vector<MidiEvent> chord;
        MidiEvent chordNote;
        chordNote.data[0] = note.data[0];
        chordNote.data[1] = note.data[1];
        chordNote.data[2] = note.data[2];
        chordNote.size = 3;
        chordNote.frame = 0;
        chord.push_back(chordNote);
        switch (currentMode)
        {
        case MidiChord::Mode::FIFTH:
            chordNote.data[1] = note.data[1] + 7;
            chord.push_back(chordNote);
            break;
        case MidiChord::Mode::FIFTH_OCT_DOWN:
            chordNote.data[1] = note.data[1] + 7;
            chord.push_back(chordNote);
            chordNote.data[1] = note.data[1] - 12;
            chord.push_back(chordNote);
            break;
        case MidiChord::Mode::FIFTH_OCT_UP:
            chordNote.data[1] = note.data[1] + 7;
            chord.push_back(chordNote);
            chordNote.data[1] = note.data[1] + 12;
            chord.push_back(chordNote);
            break;
        case MidiChord::Mode::OCT_DOWN:
            chordNote.data[1] = note.data[1] - 12;
            chord.push_back(chordNote);
            break;
        case MidiChord::Mode::OCT_UP:
            chordNote.data[1] = note.data[1] + 12;
            chord.push_back(chordNote);
            break;
        case MidiChord::Mode::OCT_UP_DOWN:
            chordNote.data[1] = note.data[1] - 12;
            chord.push_back(chordNote);
            chordNote.data[1] = note.data[1] + 12;
            chord.push_back(chordNote);
            break;
        case MidiChord::Mode::MAJOR:
            chordNote.data[1] = note.data[1] + 4;
            chord.push_back(chordNote);
            chordNote.data[1] = note.data[1] + 7;
            chord.push_back(chordNote);
            break;
        case MidiChord::Mode::MINOR:
            chordNote.data[1] = note.data[1] + 3;
            chord.push_back(chordNote);
            chordNote.data[1] = note.data[1] + 7;
            chord.push_back(chordNote);
            break;
        default:
            break;
        }
        return chord;
    } 
};

#endif // _MIDI_CHORD_H