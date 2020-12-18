#ifndef MAKE_WAV_H
#define MAKE_WAV_H

/* open a file named filename, write signed 16-bit values as a
    monoaural WAV file at the specified sampling rate
    and close the file
*/
unsigned long write_wav(char * filename, unsigned long num_samples,  int s_rate);

#endif


