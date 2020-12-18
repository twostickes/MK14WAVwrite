# MK14WAVwrite
Converts Intel HEX formated data for MK14 to audio .wav file for the tape drive.
This consists of bursts of 1khz tones in the same way MK14 + tape drive would create them for tape cassette use.
However this WAV file is free of noise, distortion, motor speed errors.

The chose IDE is Code::Blocks 20.03 and use project file MK14WAVwrite.cbp.
Code output is a windows .exe file and includes command line options.


information about the WAV file format from
    http://ccrma.stanford.edu/courses/422/projects/WaveFormat/
    
eg DUCK.HEX
:180F1200C40D35C40031C401C8F4C410C8F1C400C8EEC40801C0E71EB2
:180F2A00C8E49404C4619002C400C9808F01C0D89C0EC180E4FF980811
:160F4200C8CEC0CAE480C8C64003FC0194D6B8BF98C8C40790CEDD
:00000001FF

eg First line is formatted as follows:
StartCode   ByteCount   Address   RecordType   Data                                             Checksum
:           18          0F12      00           C40D35C40031C401C8F4C410C8F1C400C8EEC40801C0E71E B2
eg Last line is formatted as follows:
:           00          0000      01                                                            FF
