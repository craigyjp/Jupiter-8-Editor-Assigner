# Jupiter 8 Editor / Assigner

This project was put togther to control a pair of Roland boutique JP-08 synth modules.

![Synth](photos/synth.jpg)

These modules are 4 note polyphonic and can be used as half of a sound card for a Jupiter 8.

The pair togther create an 8 note polyphonic synth comparable to a Jupiter 8.

The editor send MIDI CC updates to each module when a patch is recalled or a parameter is edited.

It also includes a key assigner that takes care of whole, dual and split modes and solo, unision, poly1 and poly2 modes.

I'm going to include an arpeggiator section similar to the Jupiter 8 and patch memories.

The JP-08 modules have portamento, delay and chorus effects that are hidden from the front panel and only available via MIDI.

The VCF Bend is not practical to implement so I've replaced that with aftertouch depth and destinations.

I created a perl script to convert the factory JP-08 patches to the same format as my editor and loaded all the original 64 jupiter 8 patches.

16 banks of 64 patches/performances are now available by pressing the recall button and scrolling through the banks.

Program change 0-63 recalls patches 11-88, Program changes 64-127 recalls performances 11-88.

# Things to do

* possibly switch to sysex for 0-255 parameter changes for better resolution.
* General bug fixes/features as I find them.
* Fix the sustain pedal input.





