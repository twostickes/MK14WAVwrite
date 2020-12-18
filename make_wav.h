/* make_wav.h
 * Fri Jun 18 17:06:02 PDT 2010 Kevin Karplus
 */

#ifndef MAKE_WAV_H
#define MAKE_WAV_H

unsigned long write_wav(unsigned long size, char * filename, unsigned long num_samples,  int s_rate);
    /* open a file named filename, write signed 16-bit values as a
        monoaural WAV file at the specified sampling rate
        and close the file
    */

#endif


