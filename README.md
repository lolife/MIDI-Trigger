This project was designed to use an M5 Stack device (an Arduino-like thing)
with the MIDI Unit (https://docs.m5stack.com/en/unit/Unit-MIDI) to convert audio
triggers into MIDI notes. I used the CoreS3 (https://docs.m5stack.com/en/core/CoreS3)
but any M5 device should work.

I put the audio trigger on pin G9 on Port B (but any pin should work). The MIDI
Unit goes on Port C, which for the CoreS3 is pins 17 and 18. The output of the MIDI
Unit goes into your MIDI interface. Make sure the switch on the MIDI Unit is set
to SEPERATE.

Currently it only supports 1 trigger to 1 MIDI note. The velocity is mapped from the
trigger. The latency is extremely low. Try it out and let me know what you think!

Michael Koppelman <michael.koppelman@gmail.com>
