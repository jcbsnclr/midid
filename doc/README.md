# Concept
A configurable software synthesiser, using JACK to consume MIDI and respond with audio.

Uses a simple and concise config language to describe combinations of oscillators, and 
the tracks they are assigned to.

# UI
`midid` has a CLI:
* `-l` - list available MIDI/audio ports
* `-i [REGEX]` - connect input port to MIDI ports matching regex
* `-o [REGEX]` - connect output port to audio ports matching regex
* `-I/O/E/C [SRC]` - declare an Instrument, Oscillator, Envelope, or Channel, respectively
 (*see:* [language](Language))

